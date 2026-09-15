#include "client/ui/terminal_ui.hpp"
#include "common/json/access.hpp"
#include "common/json/parser.hpp"
#include "common/unique_fd.hpp"
#include "server/collectors/metrics_collector.hpp"
#include "server/http/http_request.hpp"
#include "server/state/shared_state.hpp"
#include "server/transport/output_queue.hpp"
#include <atomic>
#include <cmath>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <thread>
#include <unistd.h>

namespace
{
#define CHECK(expression)                                                                          \
    do                                                                                             \
    {                                                                                              \
        if (!(expression))                                                                         \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) +      \
                                     ": " #expression);                                            \
    } while (false)
using namespace web_htop;
using namespace web_htop::server;
using namespace web_htop::server::collectors;

std::string Stat(int pid, std::uint64_t ticks, std::uint64_t start, std::string name = "worker")
{
    return std::to_string(pid) + " (" + name + ") R 1 1 1 0 0 0 0 0 0 0 " + std::to_string(ticks) +
           " 0 0 0 20 0 2 0 " + std::to_string(start) + " 100000 64\n";
}

class FakeSource final : public system::LinuxSource
{
  public:
    FakeSource() : LinuxSource(ServerConfig{})
    {
    }

    std::map<std::string, system::FileResult> files;
    std::map<std::string, system::FileResult> cgroup;
    std::string identity{"1:1"};
    std::vector<int> pids{42};

    system::FileResult Proc(std::string const& relative) const override
    {
        auto it = files.find(relative);
        return it == files.end() ? system::FileResult{{}, ENOENT} : it->second;
    }

    system::FileResult Sys(std::string const&) const override
    {
        return {"2", 0};
    }

    system::FileResult Cgroup(std::string const& relative) const override
    {
        auto it = cgroup.find(relative);
        return it == cgroup.end() ? system::FileResult{{}, ENOENT} : it->second;
    }

    system::PidList Pids() const override
    {
        return {pids, 0};
    }

    int Filesystem(struct statvfs& s) const override
    {
        s.f_blocks = 100;
        s.f_frsize = 4096;
        s.f_bfree = 40;
        s.f_bavail = 30;
        return 0;
    }

    std::string CgroupIdentity() const override
    {
        return identity;
    }
};

std::shared_ptr<FakeSource> Source()
{
    auto s = std::make_shared<FakeSource>();
    s->files["stat"] = {"cpu 100 0 0 100 0 0 0 0 20 0\ncpu0 100 0 0 100 0 0 0 0\n", 0};
    s->files["meminfo"] = {"MemTotal: 100000 kB\nMemAvailable: 50000 kB\n", 0};
    s->files["loadavg"] = {"1 2 3 1/2 42", 0};
    s->files["42/stat"] = {Stat(42, 100, 10), 0};
    s->files["net/dev"] = {"header\nheader\neth0: 1000 0 0 0 0 0 0 0 2000 0 0 0 0 0 0 0\n", 0};
    s->files["diskstats"] = {"8 0 sda 100 0 1000 0 100 0 2000 0 0 100 0\n", 0};
    return s;
}

void TestFd()
{
    int raw = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    CHECK(raw >= 0);
    {
        UniqueFd first(raw);
        UniqueFd second(std::move(first));
        CHECK(!first);
        CHECK(second.Get() == raw);
        second.Reset();
        second.Reset();
    }
    CHECK(::fcntl(raw, F_GETFD) == -1 && errno == EBADF);
}

void TestQueue()
{
    auto buf = [](char const* s)
    {
        return std::make_shared<std::string const>(s);
    };
    OutputQueue queue;
    CHECK(!queue.Push(buf("first")));
    queue.Consume(2);
    CHECK(!queue.Push(buf("second")));
    CHECK(queue.Push(buf("third")));
    CHECK(queue.Front() == "rst");
    CHECK(queue.Bytes() == 8);
    queue.Consume(3);
    CHECK(queue.Front() == "third");
    CHECK(queue.Push(buf("new")));
    CHECK(queue.Front() == "new");
    queue.Consume(3);
    CHECK(queue.Empty());
}

void TestFraming()
{
    auto wire = protocol::Frame("abcdef");

    for (std::size_t split = 0; split <= wire.size(); ++split)
    {
        protocol::FrameDecoder d;
        CHECK(d.Feed(std::span(wire.data(), split)) == split);
        CHECK(d.Feed(std::span(wire.data() + split, wire.size() - split)) == wire.size() - split);
        CHECK(d.Complete());
        CHECK(d.Payload() == "abcdef");
    }
    protocol::FrameDecoder d;
    auto twice = wire + wire;
    CHECK(d.Feed(twice) == wire.size());
    d.Reset();
    CHECK(d.Feed(std::span(twice.data() + wire.size(), wire.size())) == wire.size());
    CHECK(d.Complete());

    for (auto bad : {std::string(4, '\0'), std::string(4, '\xff')})
    {
        bool threw = false;

        try
        {
            protocol::FrameDecoder f;
            (void)f.Feed(bad);
        }
        catch (std::length_error const&)
        {
            threw = true;
        }
        CHECK(threw);
    }
}

void TestJsonOwnershipAndLimits()
{
    json::Value value;
    {
        models::ProcessInfo p;
        p.name = "short-owned-name";
        value = p.ToJson();
    }
    CHECK(value["name"]->get().AsString() == "short-owned-name");
    auto parsed = json::Parse("{\"escaped\":\"a\\nb\",\"big\":18446744073709551615}");
    CHECK(parsed);
    auto moved = std::move(parsed->value);
    parsed.reset();
    CHECK(moved["escaped"]->get().AsString() == "a\nb");
    CHECK(!json::Parse(std::string(1000, '[') + "0" + std::string(1000, ']')));
    CHECK(!json::Parse("1e99999"));
    CHECK(!json::Parse("[1,]"));
    CHECK(!json::Parse("{\"a\":1,\"a\":2}"));
    CHECK(!json::Parse(std::string("\"") + char(0xff) + "\""));
    json::Value bytes(std::string_view("bad\xff", 4));
    CHECK(json::Parse(bytes.ToString()).has_value());
}

void TestCpu()
{
    auto sample =
        ParseCpu("cpu  10\t20  30 40 50 60 70 80 9000 9000\r\n\ncpu7 1 2 3 4 5 6 7 8  \n");
    CHECK(sample.size() == 2);
    CHECK(sample.contains(7));
    CHECK(sample[-1][7] == 80);
    CpuTimes previous{100, 0, 100, 100, 0, 0, 0, 0}, now{150, 0, 150, 200, 0, 0, 0, 0};
    CHECK(std::abs(*CpuUsage(now, previous) - 50) < 0.001);
    CHECK(!CpuUsage(previous, now));
    CHECK(!CpuUsage(previous, previous));
    CHECK(!CounterRate(0, 100, 1));
    CHECK(!CounterRate(100, 0, 0));
}

void TestProcessParser()
{
    auto p = ParseProcess(Stat(42, 120, 77, "a tricky ) name\nwith newline"));
    CHECK(p);
    CHECK(p->pid == 42);
    CHECK(p->ticks == 120);
    CHECK(p->starttime == 77);
    CHECK(p->threads == 2);
    CHECK(p->rss_pages == 64);
    CHECK(p->name == "a tricky ) name\nwith newline");
    CHECK(!ParseProcess("42 (broken) R 1"));
    CHECK(!ParseProcess(""));
    CHECK(!ParseProcess("not_a_pid (name) R 1 2 3"));
}

void TestCollectorAndReuse()
{
    auto source = Source();
    ServerConfig config;
    config.max_processes = 1;
    MetricsCollector collector(config, source);
    auto time = std::chrono::steady_clock::time_point{};
    auto first = collector.Collect({}, time);
    CHECK(first.process.processes.size() == 1);
    CHECK(!first.process.processes[0].cpu_valid);
    source->files["42/stat"] = {Stat(42, 200, 10), 0};
    source->files["stat"] = {"cpu 150 0 0 150 0 0 0 0\ncpu0 150 0 0 150 0 0 0 0\n", 0};
    source->files["net/dev"] = {"eth0: 2024 0 0 0 0 0 0 0 4048 0 0 0 0 0 0 0\n", 0};
    auto second = collector.Collect({}, time + std::chrono::seconds(1));
    CHECK(second.process.processes[0].cpu_valid);
    CHECK(second.cpu.total_usage_percent == 50);
    CHECK(second.network.rx_kbps == 1);
    CHECK(second.network.tx_kbps == 2);
    CHECK(second.disk.used_bytes == 60 * 4096);
    source->files["42/stat"] = {Stat(42, 999, 20), 0};
    source->files["net/dev"] = {"eth0: 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0\n", 0};
    auto third = collector.Collect({}, time + std::chrono::seconds(2));
    CHECK(!third.process.processes[0].cpu_valid);
    CHECK(!third.telemetry.interfaces[0].rx_bytes_per_second);
    source->pids = {42, 43};
    source->files["43/stat"] = {{}, EACCES};
    auto fourth = collector.Collect({}, time + std::chrono::seconds(3));
    CHECK(fourth.telemetry.process_denied == 1);
}

void TestCgroup()
{
    auto source = Source();
    ServerConfig config;
    config.cgroup_path = "fixture";
    source->cgroup["memory.current"] = {"100", 0};
    source->cgroup["memory.max"] = {"max\n", 0};
    source->cgroup["cpu.max"] = {"200000 100000", 0};
    source->cgroup["cpu.stat"] = {"usage_usec 100\nthrottled_usec 10\nnr_throttled 1", 0};
    MetricsCollector collector(config, source);
    auto time = std::chrono::steady_clock::time_point{};
    auto first = collector.Collect({}, time);
    CHECK(first.telemetry.cgroup.memory_unlimited);
    CHECK(first.telemetry.cgroup.cpu_quota_cores == 2);
    source->cgroup["cpu.stat"] = {"usage_usec 1000100\nthrottled_usec 10010\nnr_throttled 2", 0};
    auto second = collector.Collect({}, time + std::chrono::seconds(1));
    CHECK(second.telemetry.cgroup.cpu_percent == 100);
    CHECK(second.telemetry.cgroup.throttled_ms_per_second == 10);
    source->identity = "1:2";
    auto third = collector.Collect({}, time + std::chrono::seconds(2));
    CHECK(!third.telemetry.cgroup.cpu_percent);
}

void TestHttp()
{
    CHECK(ParseHttpRequest("GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n").status == 200);
    CHECK(ParseHttpRequest("GET /health HTTP/1.1\r\n\r\n").status == 400);
    CHECK(ParseHttpRequest("POST / HTTP/1.1\r\nHost: h\r\n\r\n").status == 405);
    CHECK(ParseHttpRequest("GET / HTTP/1.1\r\nHost: h\r\nContent-Length: 4\r\n\r\n").status == 400);
    CHECK(ParseHttpRequest("GET / HTTP/1.1\r\nHost: h\r\nTransfer-Encoding: chunked\r\n\r\n")
              .status == 400);
    CHECK(ParseHttpRequest("GET / HTTP/1.1\r\nHost: h\r\nHost: x\r\n\r\n").status == 400);
}

void TestPublication()
{
    SharedState state;
    std::atomic<bool> good = true;
    auto writer = std::jthread(
        [&]
        {
            for (unsigned i = 1; i <= 200; ++i)
            {
                models::SystemSnapshot s;
                s.telemetry.sequence = i;
                s.telemetry.instance_id = "test";
                s.timestamp = i;
                state.Publish(std::move(s));
            }
        });
    std::vector<std::jthread> readers;

    for (int j = 0; j < 3; ++j)
    {
        readers.emplace_back(
            [&]
            {
                for (int i = 0; i < 1000; ++i)
                {
                    if (auto s = state.Load())
                    {
                        auto parsed = json::Parse(*s->json);

                        if (!parsed || json::UInt(parsed->value, "timestamp") !=
                                           s->snapshot.telemetry.sequence)
                        {
                            good = false;
                        }
                    }
                }
            });
    }
    writer.join();

    for (auto& reader : readers)
    {
        reader.join();
    }
    CHECK(good.load());
    CHECK(state.Load()->snapshot.telemetry.sequence == 200);
}

void TestDecodeAndUi()
{
    models::SystemSnapshot s;
    s.telemetry.instance_id = "test";
    s.telemetry.sequence = 1;
    std::string error;
    auto decoded = protocol::Decode(protocol::Encode(s), error);
    CHECK(decoded);
    auto json = s.ToJson();
    CHECK(!protocol::Decode(json.ToString(), error));
    CHECK(client::ui::Sanitize("bad\033[2J\n") == "bad?[2J?");
    client::ui::ViewState v;
    v.snapshot = &s;

    for (unsigned page = 1; page <= 6; ++page)
    {
        v.page = page;
        auto screen = client::ui::Render(v, 120, 40, false);
        CHECK(screen.find('\033') == std::string::npos);
        CHECK(!screen.empty());
    }
}
} // namespace

int main()
{
    std::vector<std::pair<char const*, std::function<void()>>> tests{
        {"fd ownership", TestFd},
        {"latest-wins queue", TestQueue},
        {"frame fragmentation", TestFraming},
        {"JSON ownership and limits", TestJsonOwnershipAndLimits},
        {"CPU arithmetic", TestCpu},
        {"process stat parser", TestProcessParser},
        {"collector and PID reuse", TestCollectorAndReuse},
        {"cgroup reset", TestCgroup},
        {"HTTP subset", TestHttp},
        {"concurrent publication", TestPublication},
        {"codec and UI", TestDecodeAndUi}};
    unsigned failed = 0;

    for (auto const& [name, test] : tests)
    {
        try
        {
            test();
            std::cout << "PASS " << name << '\n';
        }
        catch (std::exception const& e)
        {
            ++failed;
            std::cerr << "FAIL " << name << ": " << e.what() << '\n';
        }
    }
    return failed ? 1 : 0;
}
