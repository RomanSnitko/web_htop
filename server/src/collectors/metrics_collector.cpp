#include "server/collectors/metrics_collector.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <numeric>
#include <sstream>
#include <system_error>
#include <unistd.h>

namespace web_htop::server::collectors
{
namespace
{
using Clock = std::chrono::steady_clock;

std::uint64_t Timestamp()
{
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::system_clock::now().time_since_epoch())
                                          .count());
}

std::string Require(system::FileResult value)
{
    if (!value)
    {
        throw std::system_error(value.error, std::generic_category());
    }
    return std::move(value.text);
}

std::map<std::string, std::uint64_t> KeyValues(std::string const& text)
{
    std::map<std::string, std::uint64_t> result;
    std::istringstream input(text);
    std::string line;

    while (std::getline(input, line))
    {
        std::istringstream row(line);
        std::string key;
        std::uint64_t value{};

        if (row >> key >> value)
        {
            if (key.ends_with(':'))
            {
                key.pop_back();
            }
            result[key] = value;
        }
    }
    return result;
}

std::optional<std::uint64_t> Scalar(system::FileResult const& f)
{
    if (!f)
    {
        return std::nullopt;
    }
    auto s = std::string_view(f.text);

    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
    {
        s.remove_suffix(1);
    }
    std::uint64_t n{};
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), n);
    return ec == std::errc{} && p == s.data() + s.size() ? std::optional(n) : std::nullopt;
}

std::string Trim(std::string s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
    {
        s.pop_back();
    }
    return s;
}

void State(models::SystemSnapshot& s, std::string state)
{
    s.telemetry.collectors.back().state = std::move(state);
}
} // namespace

MetricsCollector::MetricsCollector(ServerConfig c,
                                   std::shared_ptr<system::LinuxSource const> source)
    : config_(std::move(c)), source_(std::move(source)), clock_ticks_(::sysconf(_SC_CLK_TCK)),
      page_size_(::sysconf(_SC_PAGESIZE))
{
    if (clock_ticks_ <= 0 || page_size_ <= 0)
    {
        throw std::runtime_error("sysconf clock/page size unavailable");
    }
    if (auto b = source_->Proc("sys/kernel/random/boot_id"))
    {
        boot_id_ = Trim(b.text);
    }
    char host[256]{};

    if (::gethostname(host, sizeof(host) - 1) == 0)
    {
        hostname_ = host;
    }
}

models::SystemSnapshot MetricsCollector::Collect(std::stop_token stop, Clock::time_point now)
{
    models::SystemSnapshot s;
    s.timestamp = Timestamp();
    s.telemetry.collection_started_at = s.timestamp;
    s.telemetry.interval_ms = static_cast<std::uint64_t>(config_.poll_interval.count());
    s.telemetry.boot_id = boot_id_;
    s.telemetry.hostname = hostname_;
    seconds_ = previous_time_ ? std::chrono::duration<double>(now - *previous_time_).count() : 0;
    auto started = Clock::now();
    auto collect = [&](std::string name, auto action)
    {
        s.telemetry.collectors.push_back({std::move(name), "ok", {}, 0});
        auto begin = Clock::now();

        try
        {
            action();
        }
        catch (std::exception const& e)
        {
            auto& status = s.telemetry.collectors.back();
            status.state = "unavailable";
            status.error = e.what();
        }
        s.telemetry.collectors.back().duration_us = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - begin).count());
    };
    collect("cpu",
            [&]
            {
                Cpu(s);
            });
    collect("memory",
            [&]
            {
                Memory(s);
            });
    collect("processes",
            [&]
            {
                Processes(s, stop);
            });
    collect("network",
            [&]
            {
                Network(s);
            });
    collect("disk_io",
            [&]
            {
                Disk(s);
            });
    collect("filesystem",
            [&]
            {
                Filesystem(s);
            });
    collect("load",
            [&]
            {
                Load(s);
            });
    collect("psi",
            [&]
            {
                Pressure(s);
            });

    if (!config_.cgroup_path.empty())
    {
        collect("cgroup",
                [&]
                {
                    Cgroup(s);
                });
    }
    previous_time_ = now;
    s.telemetry.collection_finished_at = Timestamp();
    s.telemetry.collection_us = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - started).count());
    return s;
}

void MetricsCollector::Cpu(models::SystemSnapshot& s)
{
    auto file = source_->Proc("stat");

    if (!file)
    {
        cpu_previous_.clear();
        Require(std::move(file));
    }
    auto now = ParseCpu(file.text);

    if (!now.contains(-1))
    {
        cpu_previous_.clear();
        throw std::runtime_error("aggregate CPU line missing or malformed");
    }
    s.cpu.timestamp = s.timestamp;
    s.cpu.core_count = static_cast<CPUCores>(now.size() - 1);
    bool all_valid = true;

    for (auto const& [id, times] : now)
    {
        auto previous = cpu_previous_.find(id);
        auto usage =
            previous == cpu_previous_.end() ? std::nullopt : CpuUsage(times, previous->second);
        all_valid &= usage.has_value();

        if (id == -1)
        {
            s.cpu.total_usage_percent = usage.value_or(0);
        }
        else
        {
            s.telemetry.cpu_ids.push_back(static_cast<unsigned>(id));
            s.cpu.per_core_usage_percent.push_back(usage.value_or(0));
        }
    }
    if (!all_valid)
    {
        State(s, cpu_previous_.empty() ? "warming_up" : "partial");
    }
    cpu_previous_ = std::move(now);
}

void MetricsCollector::Memory(models::SystemSnapshot& s)
{
    auto values = KeyValues(Require(source_->Proc("meminfo")));

    if (!values.contains("MemTotal") || !values.contains("MemAvailable") ||
        values["MemTotal"] == 0 || values["MemAvailable"] > values["MemTotal"] ||
        values["MemTotal"] > std::numeric_limits<std::uint64_t>::max() / 1024)
    {
        throw std::runtime_error("MemTotal/MemAvailable missing or invalid");
    }
    s.memory.timestamp = s.timestamp;
    s.memory.total_bytes = values["MemTotal"] * 1024;
    s.memory.available_bytes = values["MemAvailable"] * 1024;
    s.memory.used_bytes = s.memory.total_bytes - s.memory.available_bytes;
    s.memory.used_percent = 100.0 * static_cast<double>(s.memory.used_bytes) /
                            static_cast<double>(s.memory.total_bytes);
}

void MetricsCollector::Processes(models::SystemSnapshot& s, std::stop_token stop)
{
    auto list = source_->Pids();

    if (list.error)
    {
        processes_previous_.clear();
        throw std::system_error(list.error, std::generic_category());
    }
    s.process.timestamp = s.timestamp;
    std::unordered_map<int, std::pair<std::uint64_t, std::uint64_t>> next;
    next.reserve(list.pids.size());
    s.process.processes.reserve(std::min(list.pids.size(), config_.max_processes));

    for (int pid : list.pids)
    {
        if (stop.stop_requested())
        {
            State(s, "partial");
            break;
        }
        auto file = source_->Proc(std::to_string(pid) + "/stat");

        if (!file)
        {
            if (file.error == EACCES || file.error == EPERM)
            {
                ++s.telemetry.process_denied;
            }
            else if (file.error == ENOENT || file.error == ESRCH)
            {
                ++s.telemetry.process_vanished;
            }
            else
            {
                ++s.telemetry.process_malformed;
            }
            continue;
        }
        auto p = ParseProcess(file.text);

        if (!p || p->pid != pid ||
            p->rss_pages >
                std::numeric_limits<std::uint64_t>::max() / static_cast<std::uint64_t>(page_size_))
        {
            ++s.telemetry.process_malformed;
            continue;
        }
        models::ProcessInfo info;
        info.pid = pid;
        info.name = p->name;
        info.state = static_cast<ProcessState>(p->state);
        info.starttime_ticks = p->starttime;
        info.thread_count = p->threads;
        info.memory_bytes = p->rss_pages * static_cast<std::uint64_t>(page_size_);

        if (s.memory.total_bytes)
        {
            info.memory_percent = 100.0 * static_cast<double>(info.memory_bytes) /
                                  static_cast<double>(s.memory.total_bytes);
        }
        if (auto old = processes_previous_.find(pid);
            old != processes_previous_.end() && old->second.first == p->starttime)
        {
            if (auto rate = CounterRate(p->ticks, old->second.second, seconds_))
            {
                info.cpu_percent = *rate * 100.0 / static_cast<double>(clock_ticks_);
                info.cpu_valid = true;
            }
        }
        next[pid] = {p->starttime, p->ticks};
        ++s.process.total_processes;

        if (p->state == 'R')
        {
            ++s.process.running_processes;
        }
        s.process.processes.push_back(std::move(info));
    }
    processes_previous_ = std::move(next);
    auto better = [](auto const& a, auto const& b)
    {
        if (a.cpu_percent != b.cpu_percent)
        {
            return a.cpu_percent > b.cpu_percent;
        }
        if (a.memory_bytes != b.memory_bytes)
        {
            return a.memory_bytes > b.memory_bytes;
        }
        return a.pid < b.pid;
    };
    auto count = std::min(config_.max_processes, s.process.processes.size());
    std::partial_sort(s.process.processes.begin(),
                      s.process.processes.begin() + static_cast<std::ptrdiff_t>(count),
                      s.process.processes.end(), better);
    s.telemetry.processes_truncated = count < s.process.processes.size();
    s.process.processes.resize(count);

    if (s.telemetry.process_denied || s.telemetry.process_malformed)
    {
        State(s, "partial");
    }
}

void MetricsCollector::Network(models::SystemSnapshot& s)
{
    auto f = source_->Proc("net/dev");

    if (!f)
    {
        network_previous_.clear();
        Require(std::move(f));
    }
    std::istringstream input(f.text);
    std::string line;
    std::map<std::string, NetSample> next;
    bool valid = true;
    s.network.timestamp = s.timestamp;

    while (std::getline(input, line))
    {
        auto colon = line.find(':');

        if (colon == std::string::npos)
        {
            continue;
        }
        std::string name = Trim(line.substr(0, colon));
        auto begin = name.find_first_not_of(" \t");

        if (begin == std::string::npos)
        {
            continue;
        }
        name.erase(0, begin);

        if (name == "lo" && !config_.include_loopback)
        {
            continue;
        }
        std::istringstream row(line.substr(colon + 1));
        std::array<std::uint64_t, 16> fields{};
        bool parsed = true;

        for (auto& n : fields)
        {
            if (!(row >> n))
            {
                parsed = false;
                break;
            }
        }
        if (!parsed)
        {
            valid = false;
            continue;
        }
        auto id = source_->Sys("class/net/" + name + "/ifindex");
        std::string identity = id ? Trim(id.text) : name;
        models::InterfaceMetrics m;
        m.name = name;
        m.rx_bytes = fields[0];
        m.tx_bytes = fields[8];
        m.rx_errors = fields[2];
        m.tx_errors = fields[10];
        m.rx_dropped = fields[3];
        m.tx_dropped = fields[11];

        if (auto p = network_previous_.find(name);
            p != network_previous_.end() && p->second.identity == identity)
        {
            m.rx_bytes_per_second = CounterRate(fields[0], p->second.rx, seconds_);
            m.tx_bytes_per_second = CounterRate(fields[8], p->second.tx, seconds_);
        }
        valid &= m.rx_bytes_per_second.has_value() && m.tx_bytes_per_second.has_value();
        s.network.rx_bytes_total += fields[0];
        s.network.tx_bytes_total += fields[8];
        s.network.rx_kbps += m.rx_bytes_per_second.value_or(0) / 1024.0;
        s.network.tx_kbps += m.tx_bytes_per_second.value_or(0) / 1024.0;
        next[name] = {std::move(identity), fields[0], fields[8]};
        s.telemetry.interfaces.push_back(std::move(m));
    }
    if (!valid)
    {
        State(s, network_previous_.empty() ? "warming_up" : "partial");
    }
    network_previous_ = std::move(next);
}

void MetricsCollector::Disk(models::SystemSnapshot& s)
{
    auto f = source_->Proc("diskstats");

    if (!f)
    {
        disks_previous_.clear();
        Require(std::move(f));
    }
    std::istringstream input(f.text);
    std::string line;
    decltype(disks_previous_) next;

    while (std::getline(input, line))
    {
        std::istringstream row(line);
        unsigned major{}, minor{};
        std::string name;
        std::array<std::uint64_t, 11> fields{};

        if (!(row >> major >> minor >> name))
        {
            continue;
        }
        bool valid = true;

        for (auto& n : fields)
        {
            if (!(row >> n))
            {
                valid = false;
                break;
            }
        }
        if (!valid)
        {
            State(s, "partial");
            continue;
        }
        // Partitions and stacked devices stay separate. Summing them double-counts I/O.
        std::string key = std::to_string(major) + ":" + std::to_string(minor) + ":" + name;
        std::array<std::uint64_t, 5> counters{fields[2], fields[6], fields[0], fields[4],
                                              fields[9]};
        models::DiskIoMetrics m;
        m.device = name;

        if (auto old = disks_previous_.find(key); old != disks_previous_.end())
        {
            auto rd = CounterRate(counters[0], old->second[0], seconds_),
                 wr = CounterRate(counters[1], old->second[1], seconds_);

            if (rd)
            {
                m.read_bytes_per_second = *rd * 512.0;
            }
            if (wr)
            {
                m.write_bytes_per_second = *wr * 512.0;
            }
            auto reads = CounterRate(counters[2], old->second[2], seconds_),
                 writes = CounterRate(counters[3], old->second[3], seconds_);

            if (reads && writes)
            {
                m.iops = *reads + *writes;
            }
            if (auto busy = CounterRate(counters[4], old->second[4], seconds_))
            {
                m.busy_percent = std::min(100.0, *busy / 10.0);
            }
        }
        next[key] = counters;
        s.telemetry.disks.push_back(std::move(m));
    }
    if (disks_previous_.empty() && !next.empty())
    {
        State(s, "warming_up");
    }
    disks_previous_ = std::move(next);
}

void MetricsCollector::Filesystem(models::SystemSnapshot& s)
{
    struct statvfs st
    {
    };

    int ec = source_->Filesystem(st);

    if (ec)
    {
        throw std::system_error(ec, std::generic_category());
    }
    s.disk.timestamp = s.timestamp;
    s.disk.total_bytes = st.f_blocks * st.f_frsize;
    s.disk.available_bytes = st.f_bavail * st.f_frsize;
    s.disk.used_bytes = (st.f_blocks >= st.f_bfree ? st.f_blocks - st.f_bfree : 0) * st.f_frsize;

    if (s.disk.total_bytes)
    {
        s.disk.used_percent = 100.0 * static_cast<double>(s.disk.used_bytes) /
                              static_cast<double>(s.disk.total_bytes);
    }
}

void MetricsCollector::Load(models::SystemSnapshot& s)
{
    auto text = Require(source_->Proc("loadavg"));
    std::string_view sv = text;
    s.loadavg.timestamp = s.timestamp;

    auto parse_next = [](std::string_view& v, double& out) -> bool
    {
        while (!v.empty() && std::isspace(static_cast<unsigned char>(v.front())))
        {
            v.remove_prefix(1);
        }
        if (v.empty())
        {
            return false;
        }
        auto end = v.find_first_of(" \t\n\r");
        std::string_view token = (end == std::string_view::npos) ? v : v.substr(0, end);
        auto [p, ec] = std::from_chars(token.data(), token.data() + token.size(), out);
        if (ec != std::errc{} || p != token.data() + token.size())
        {
            return false;
        }
        v.remove_prefix(token.size());
        return true;
    };

    if (!parse_next(sv, s.loadavg.load_1m) || !parse_next(sv, s.loadavg.load_5m) ||
        !parse_next(sv, s.loadavg.load_15m))
    {
        throw std::runtime_error("malformed loadavg");
    }
}

void MetricsCollector::Pressure(models::SystemSnapshot& s)
{
    unsigned available = 0;

    for (auto const* resource : {"cpu", "memory", "io"})
    {
        auto f = source_->Proc(std::string("pressure/") + resource);
        auto p = ParsePressure(resource, f ? f.text : "");

        if (p.some_avg10)
        {
            ++available;
        }
        s.telemetry.pressure.push_back(std::move(p));
    }
    if (available < 3)
    {
        State(s, available ? "partial" : "unavailable");
    }
}

void MetricsCollector::Cgroup(models::SystemSnapshot& s)
{
    auto& c = s.telemetry.cgroup;
    c.path = config_.cgroup_path;
    c.state = "unavailable";
    const auto identity = source_->CgroupIdentity();

    if (identity.empty() || identity != cg_identity_)
    {
        cg_usage_.reset();
        cg_throttled_.reset();
    }
    cg_identity_ = identity;

    if (identity.empty())
    {
        State(s, "unavailable");
        return;
    }
    c.memory_current = Scalar(source_->Cgroup("memory.current"));
    auto mem_max = source_->Cgroup("memory.max");
    c.memory_max = Scalar(mem_max);
    c.memory_unlimited = mem_max && Trim(mem_max.text) == "max";
    auto events = source_->Cgroup("memory.events");

    if (events)
    {
        auto kv = KeyValues(events.text);

        if (kv.contains("oom_kill"))
        {
            c.oom_kill = kv["oom_kill"];
        }
    }
    auto max = source_->Cgroup("cpu.max");

    if (max)
    {
        std::istringstream input(max.text);
        std::string quota;
        std::uint64_t period{};

        if (input >> quota >> period; period > 0)
        {
            c.cpu_unlimited = quota == "max";

            if (auto q = Scalar({quota, 0}))
            {
                c.cpu_quota_cores = static_cast<double>(*q) / static_cast<double>(period);
            }
        }
    }
    auto stat = source_->Cgroup("cpu.stat");

    if (stat)
    {
        auto values = KeyValues(stat.text);

        if (values.contains("usage_usec"))
        {
            auto current = values["usage_usec"];

            if (cg_usage_)
            {
                if (auto rate = CounterRate(current, *cg_usage_, seconds_))
                {
                    c.cpu_percent = *rate / 10000.0;
                }
            }
            cg_usage_ = current;
        }
        else
        {
            cg_usage_.reset();
        }
        if (values.contains("throttled_usec"))
        {
            auto current = values["throttled_usec"];

            if (cg_throttled_)
            {
                if (auto rate = CounterRate(current, *cg_throttled_, seconds_))
                {
                    c.throttled_ms_per_second = *rate / 1000.0;
                }
            }
            cg_throttled_ = current;
        }
        else
        {
            cg_throttled_.reset();
        }
        if (values.contains("nr_throttled"))
        {
            c.nr_throttled = values["nr_throttled"];
        }
    }
    else
    {
        cg_usage_.reset();
        cg_throttled_.reset();
    }
    for (auto const* resource : {"cpu", "memory", "io"})
    {
        auto f = source_->Cgroup(std::string(resource) + ".pressure");
        c.pressure.push_back(ParsePressure(resource, f ? f.text : ""));
    }
    c.state =
        c.memory_current && stat ? "ok" : (c.memory_current || stat ? "partial" : "unavailable");
    State(s, c.state);
}
} // namespace web_htop::server::collectors
