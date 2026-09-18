#include "server/transport/reactor.hpp"
#include "common/json/access.hpp"
#include "server/http/http_request.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <filesystem>
#include <iostream>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <system_error>

namespace web_htop::server
{
namespace
{
constexpr std::uint64_t kHttp = 1, kStream = 2, kSignal = 3, kNotification = 4, kTimer = 5;
constexpr std::size_t kWriteBudget = 64 * 1024, kQueueBudget = 64 * 1024 * 1024;

void Check(int result, char const* operation)
{
    if (result < 0)
    {
        throw std::system_error(errno, std::generic_category(), operation);
    }
}

UniqueFd Listen(std::string const& address, unsigned port)
{
    sockaddr_storage storage{};
    socklen_t length{};
    auto* v4 = reinterpret_cast<sockaddr_in*>(&storage);
    auto* v6 = reinterpret_cast<sockaddr_in6*>(&storage);
    int family = AF_INET;

    if (::inet_pton(AF_INET, address.c_str(), &v4->sin_addr) == 1)
    {
        v4->sin_family = AF_INET;
        v4->sin_port = htons(static_cast<std::uint16_t>(port));
        length = sizeof(*v4);
    }
    else if (::inet_pton(AF_INET6, address.c_str(), &v6->sin6_addr) == 1)
    {
        family = AF_INET6;
        v6->sin6_family = AF_INET6;
        v6->sin6_port = htons(static_cast<std::uint16_t>(port));
        length = sizeof(*v6);
    }
    else
    {
        throw std::invalid_argument("--bind requires a numeric IPv4 or IPv6 address");
    }
    UniqueFd fd(::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0));
    Check(fd.Get(), "socket");
    int yes = 1;
    Check(::setsockopt(fd.Get(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)), "SO_REUSEADDR");

    if (family == AF_INET6)
    {
        Check(::setsockopt(fd.Get(), IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes)), "IPV6_V6ONLY");
    }
    Check(::bind(fd.Get(), reinterpret_cast<sockaddr*>(&storage), length), "bind");
    Check(::listen(fd.Get(), 128), "listen");
    return fd;
}
} // namespace

Reactor::Reactor(ServerConfig c, SharedState const& state, int signal_fd, int notification_fd)
    : config_(std::move(c)), state_(state), signal_fd_(signal_fd),
      notification_fd_(notification_fd), epoll_(::epoll_create1(EPOLL_CLOEXEC)),
      http_listener_(Listen(config_.bind_address, config_.port)),
      stream_listener_(Listen(config_.bind_address, config_.streaming_port)),
      timer_(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC))
{
    Check(epoll_.Get(), "epoll_create1");
    Check(timer_.Get(), "timerfd_create");
    itimerspec interval{};
    interval.it_value.tv_nsec = 100000000;
    interval.it_interval = interval.it_value;
    Check(::timerfd_settime(timer_.Get(), 0, &interval, nullptr), "timerfd_settime");
    Register(http_listener_.Get(), kHttp, EPOLLIN);
    Register(stream_listener_.Get(), kStream, EPOLLIN);
    Register(signal_fd_, kSignal, EPOLLIN);
    Register(notification_fd_, kNotification, EPOLLIN);
    Register(timer_.Get(), kTimer, EPOLLIN);
}

void Reactor::Register(int fd, std::uint64_t token, std::uint32_t events)
{
    epoll_event e{};
    e.events = events;
    e.data.u64 = token;
    Check(::epoll_ctl(epoll_.Get(), EPOLL_CTL_ADD, fd, &e), "epoll add");
}

void Reactor::Modify(std::uint64_t token, Session const& session)
{
    epoll_event e{};
    e.data.u64 = token;
    // Stream is server-to-client only. Not watching EPOLLIN avoids an EOF wakeup loop
    // when a client shuts down its unused write half.
    e.events =
        (!session.streaming && !session.responding ? static_cast<unsigned>(EPOLLIN | EPOLLRDHUP)
                                                   : 0u) |
        (!session.output.Empty() ? static_cast<unsigned>(EPOLLOUT) : 0u);
    Check(::epoll_ctl(epoll_.Get(), EPOLL_CTL_MOD, session.fd.Get(), &e), "epoll modify");
}

void Reactor::Accept(bool streaming)
{
    int listener = streaming ? stream_listener_.Get() : http_listener_.Get();

    for (unsigned count = 0; count < 32; ++count)
    {
        UniqueFd fd(::accept4(listener, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC));

        if (!fd)
        {
            if (errno == EINTR)
            {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return;
            }
            if (errno == EMFILE || errno == ENFILE)
            {
                (void)::epoll_ctl(epoll_.Get(), EPOLL_CTL_DEL, http_listener_.Get(), nullptr);
                (void)::epoll_ctl(epoll_.Get(), EPOLL_CTL_DEL, stream_listener_.Get(), nullptr);
                listeners_paused_ = true;
            }
            ++rejected_;
            return;
        }
        if (sessions_.size() >= config_.max_clients)
        {
            ++rejected_;
            continue;
        }
        const auto token = next_token_++;
        Session s{std::move(fd), streaming, false, {}, {}, Clock::now(), Clock::now()};

        if (streaming)
        {
            int send_buffer = 64 * 1024;
            Check(
                ::setsockopt(s.fd.Get(), SOL_SOCKET, SO_SNDBUF, &send_buffer, sizeof(send_buffer)),
                "SO_SNDBUF");

            if (auto latest = state_.Load(); latest && !Enqueue(s, latest->frame))
            {
                ++rejected_;
                continue;
            }
        }
        Register(s.fd.Get(), token,
                 streaming ? (s.output.Empty() ? 0u : static_cast<unsigned>(EPOLLOUT))
                           : static_cast<unsigned>(EPOLLIN | EPOLLRDHUP));
        sessions_.emplace(token, std::move(s));
        ++accepted_;
    }
}

bool Reactor::Enqueue(Session& s, OutputQueue::Buffer buffer)
{
    const auto before = s.output.Bytes();
    bool was_empty = s.output.Empty();

    if (s.output.Push(std::move(buffer)))
    {
        ++dropped_;
    }
    queued_bytes_ = queued_bytes_ - before + s.output.Bytes();

    if (was_empty)
    {
        s.last_progress = Clock::now();
    }
    if (queued_bytes_ > kQueueBudget)
    {
        queued_bytes_ -= s.output.Bytes();
        s.output = OutputQueue{};
        return false;
    }
    return true;
}

bool Reactor::Read(Session& s)
{
    std::array<char, 2048> buffer{};

    for (unsigned tries = 0; tries < 8; ++tries)
    {
        auto n = ::recv(s.fd.Get(), buffer.data(), buffer.size(), 0);

        if (n == 0)
        {
            return false;
        }
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return errno == EAGAIN || errno == EWOULDBLOCK;
        }
        s.request.append(buffer.data(), static_cast<std::size_t>(n));
        std::string response;

        if (s.request.size() > 8192)
        {
            response = HttpResponse(431, R"({"error":"header_limit"})");
        }
        else if (s.request.find("\r\n\r\n") != std::string::npos)
        {
            response = Route(s.request);
        }
        if (!response.empty())
        {
            s.responding = true;
            s.request.clear();
            return Enqueue(s, std::make_shared<std::string const>(std::move(response)));
        }
    }
    return true;
}

bool Reactor::Write(Session& s)
{
    std::size_t budget = kWriteBudget;

    while (budget && !s.output.Empty())
    {
        auto bytes = s.output.Front();
        auto n = ::send(s.fd.Get(), bytes.data(), std::min(bytes.size(), budget), MSG_NOSIGNAL);

        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return errno == EAGAIN || errno == EWOULDBLOCK;
        }
        if (n == 0)
        {
            return false;
        }
        auto written = static_cast<std::size_t>(n);
        s.output.Consume(written);
        queued_bytes_ -= written;
        bytes_sent_ += written;
        budget -= written;
        s.last_progress = Clock::now();
    }
    return s.streaming || !s.output.Empty();
}

void Reactor::Close(std::uint64_t token)
{
    auto it = sessions_.find(token);

    if (it == sessions_.end())
    {
        return;
    }
    queued_bytes_ -= it->second.output.Bytes();
    (void)::epoll_ctl(epoll_.Get(), EPOLL_CTL_DEL, it->second.fd.Get(), nullptr);
    sessions_.erase(it);
}

void Reactor::Broadcast()
{
    auto latest = state_.Load();

    if (!latest || latest->snapshot.telemetry.sequence == last_sequence_)
    {
        return;
    }
    last_sequence_ = latest->snapshot.telemetry.sequence;

    for (auto it = sessions_.begin(); it != sessions_.end();)
    {
        auto token = it->first;
        auto& s = it++->second;

        if (!s.streaming)
        {
            continue;
        }
        if (!Enqueue(s, latest->frame))
        {
            ++rejected_;
            Close(token);
        }
        else
        {
            Modify(token, s);
        }
    }
}

void Reactor::Sweep()
{
    auto now = Clock::now();

    for (auto it = sessions_.begin(); it != sessions_.end();)
    {
        auto token = it->first;
        auto const& s = it++->second;
        bool expired =
            (!s.streaming && !s.responding && now - s.accepted_at > config_.request_timeout) ||
            (!s.output.Empty() && now - s.last_progress > config_.write_timeout);

        if (expired)
        {
            ++timeouts_;
            Close(token);
        }
    }
    if (listeners_paused_)
    {
        Register(http_listener_.Get(), kHttp, EPOLLIN);
        Register(stream_listener_.Get(), kStream, EPOLLIN);
        listeners_paused_ = false;
    }
}

std::string Reactor::Diagnostics(bool prometheus) const
{
    auto latest = state_.Load();
    auto age = latest ? std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() -
                                                                              latest->published_at)
                            .count()
                      : -1;

    if (prometheus)
    {
        return "# TYPE web_htop_sessions gauge\nweb_htop_sessions " +
               std::to_string(sessions_.size()) +
               "\n# TYPE web_htop_queued_bytes gauge\nweb_htop_queued_bytes " +
               std::to_string(queued_bytes_) +
               "\n# TYPE web_htop_snapshots_dropped_total "
               "counter\nweb_htop_snapshots_dropped_total " +
               std::to_string(dropped_) +
               "\n# TYPE web_htop_connections_rejected_total "
               "counter\nweb_htop_connections_rejected_total " +
               std::to_string(rejected_) +
               "\n# TYPE web_htop_bytes_sent_total counter\nweb_htop_bytes_sent_total " +
               std::to_string(bytes_sent_) +
               "\n# TYPE web_htop_snapshot_age_ms gauge\nweb_htop_snapshot_age_ms " +
               std::to_string(age) + "\n";
    }
    json::Object o;

    struct rusage usage
    {
    };

    if (::getrusage(RUSAGE_SELF, &usage) == 0)
    {
        auto cpu_us =
            static_cast<std::uint64_t>(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000000 +
            static_cast<std::uint64_t>(usage.ru_utime.tv_usec + usage.ru_stime.tv_usec);
        json::Add(o, "process_cpu_us", cpu_us);
        json::Add(o, "rss_peak_bytes", static_cast<std::uint64_t>(usage.ru_maxrss) * 1024);
    }
    std::error_code ec;
    std::uint64_t descriptors = 0;

    for (std::filesystem::directory_iterator it("/proc/self/fd", ec), end; !ec && it != end;
         it.increment(ec))
    {
        ++descriptors;
    }
    // The enumeration itself owns one descriptor. Do not report it as a leak.
    if (!ec && descriptors)
    {
        json::Add(o, "open_fds", descriptors - 1);
    }

    json::Add(o, "active_sessions", sessions_.size());
    json::Add(o, "accepted_total", accepted_);
    json::Add(o, "rejected_total", rejected_);
    json::Add(o, "dropped_snapshots_total", dropped_);
    json::Add(o, "timeouts_total", timeouts_);
    json::Add(o, "bytes_sent_total", bytes_sent_);
    json::Add(o, "queued_bytes", queued_bytes_);
    json::Add(o, "queue_budget_bytes", kQueueBudget);
    json::Add(o, "snapshot_age_ms", age);
    json::Add(o, "snapshot_store_lock_free", state_.IsLockFree());
    json::Add(o, "max_clients", config_.max_clients);
    json::Add(o, "bind", config_.bind_address);
    json::Add(
        o, "uptime_ms",
        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started_).count());
    json::Add(o, "encode_us", latest ? latest->encode_us : 0);
    return json::Value(std::move(o)).ToString();
}

std::string Reactor::Route(std::string_view text)
{
    const auto request = ParseHttpRequest(text);

    if (request.status != 200)
    {
        return HttpResponse(request.status, R"({"error":"invalid_request"})");
    }
    auto latest = state_.Load();

    if (request.path == "/health")
    {
        return HttpResponse(200, R"({"status":"alive","protocol_version":2})");
    }
    if (request.path == "/diagnostics")
    {
        return HttpResponse(200, Diagnostics(false));
    }
    if (request.path == "/exporter")
    {
        return HttpResponse(200, Diagnostics(true), "text/plain; version=0.0.4");
    }
    if (request.path == "/ready")
    {
        bool ready = latest && Clock::now() - latest->published_at < 3 * config_.poll_interval;

        if (latest)
        {
            for (auto const& c : latest->snapshot.telemetry.collectors)
            {
                if ((c.name == "cpu" || c.name == "memory" || c.name == "processes") &&
                    c.state != "ok")
                {
                    ready = false;
                }
            }
        }
        return HttpResponse(ready ? 200 : 503,
                            ready ? R"({"status":"ready"})" : R"({"status":"degraded"})");
    }
    if (request.path != "/metrics" && request.path != "/processes")
    {
        return HttpResponse(404, R"({"error":"not_found"})");
    }
    if (!latest)
    {
        return HttpResponse(503, R"({"error":"warming_up"})");
    }
    return HttpResponse(200, request.path == "/metrics" ? *latest->json : *latest->processes_json);
}

void Reactor::Run()
{
    std::array<epoll_event, 64> events{};

    for (;;)
    {
        int count = ::epoll_wait(epoll_.Get(), events.data(), static_cast<int>(events.size()), -1);

        if (count < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            Check(count, "epoll_wait");
        }
        // Handle shutdown before client work, including an accept flood.
        for (int i = 0; i < count; ++i)
        {
            if (events[static_cast<std::size_t>(i)].data.u64 == kSignal)
            {
                signalfd_siginfo signal{};
                (void)::read(signal_fd_, &signal, sizeof(signal));
                sessions_.clear();
                queued_bytes_ = 0;
                return;
            }
        }
        for (int i = 0; i < count; ++i)
        {
            auto event = events[static_cast<std::size_t>(i)];
            auto token = event.data.u64;

            if (token == kHttp || token == kStream)
            {
                if (!listeners_paused_)
                {
                    Accept(token == kStream);
                }
                continue;
            }
            if (token == kNotification || token == kTimer)
            {
                std::uint64_t value{};
                int fd = token == kTimer ? timer_.Get() : notification_fd_;

                while (::read(fd, &value, sizeof(value)) < 0 && errno == EINTR)
                {
                }
                if (token == kTimer)
                {
                    Sweep();
                    Broadcast();
                }
                else
                {
                    Broadcast();
                }
                continue;
            }
            auto it = sessions_.find(token);

            if (it == sessions_.end())
            {
                continue;
            }
            auto& s = it->second;
            bool alive = (event.events & EPOLLERR) == 0;

            if (alive && !s.streaming && !s.responding &&
                (event.events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP)))
            {
                alive = Read(s);
            }
            if (alive && (event.events & EPOLLOUT))
            {
                alive = Write(s);
            }
            if (event.events & EPOLLHUP)
            {
                alive = false;
            }
            if (!alive)
            {
                Close(token);
            }
            else
            {
                Modify(token, s);
            }
        }
    }
}
} // namespace web_htop::server
