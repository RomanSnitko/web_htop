/**
 * @file server/src/collectors/cpu_collector.cpp
 *
 * @author Roman Snitko
 * @date 2026-04-17
 *
 * @brief CpuCollector implementation.
 */

#include "server/collectors/cpu_collector.hpp"

#include <chrono>
#include <fstream>

namespace web_htop::server::collectors {

web_htop::models::CPUMetrics CpuCollector::Collect() {
    const CpuSnapshot current = ReadProcStat();

    web_htop::models::CPUMetrics metrics{};
    metrics.timestamp = CurrentTimestamp();
    metrics.core_count = static_cast<web_htop::CPUCores>(current.per_core.size());
    metrics.frequency_mhz = ReadFrequencyMHz();

    if (previous_snapshot_) {
        metrics.total_usage_percent =
            CalculateUsage(current.total, previous_snapshot_->total);
        metrics.per_core_usage_percent.clear();
        metrics.per_core_usage_percent.reserve(current.per_core.size());
        for (std::size_t i = 0; i < current.per_core.size(); ++i) {
            double core_usage = 0.0;
            if (i < previous_snapshot_->per_core.size()) {
                core_usage =
                    CalculateUsage(current.per_core[i], previous_snapshot_->per_core[i]);
            }
            metrics.per_core_usage_percent.push_back(core_usage);
        }
    } else {
        metrics.total_usage_percent = 0.0;
        metrics.per_core_usage_percent.assign(current.per_core.size(), 0.0);
    }

    previous_snapshot_ = current;
    return metrics;
}

std::uint64_t CpuCollector::CurrentTimestamp() const {
    const auto now = std::chrono::system_clock::now();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

std::uint64_t CpuCollector::CpuTimes::Total() const {
    return user + nice + system + idle + iowait + irq + softirq + steal;
}

std::uint64_t CpuCollector::CpuTimes::IdleTotal() const {
    return idle + iowait;
}

CpuCollector::CpuTimes CpuCollector::ParseCpuLine(const std::string& line) const {
    std::istringstream iss(line);
    std::string cpu_label;
    CpuTimes times{};
    iss >> cpu_label >> times.user >> times.nice >> times.system >> times.idle >> times.iowait >>
        times.irq >> times.softirq >> times.steal;
    return times;
}

CpuCollector::CpuSnapshot CpuCollector::ReadProcStatInternal() const {
    std::ifstream file("/proc/stat");
    std::string line;
    CpuSnapshot snapshot{};

    while (std::getline(file, line)) {
        if (line.rfind("cpu ", 0) == 0) {
            snapshot.total = ParseCpuLine(line);
        } else if (line.rfind("cpu", 0) == 0) {
            snapshot.per_core.push_back(ParseCpuLine(line));
        }
    }
    return snapshot;
}

CpuCollector::CpuSnapshot CpuCollector::ReadProcStat() const {
    return ReadProcStatInternal();
}

double CpuCollector::CalculateUsage(CpuTimes const& current, CpuTimes const& previous) const {
    const std::uint64_t total_delta = current.Total() - previous.Total();
    const std::uint64_t idle_delta = current.IdleTotal() - previous.IdleTotal();
    if (total_delta == 0) {
        return 0.0;
    }
    return (static_cast<double>(total_delta - idle_delta) / static_cast<double>(total_delta)) *
           100.0;
}

double CpuCollector::ReadFrequencyMHz() const {
    std::ifstream file("/proc/cpuinfo");
    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("cpu MHz", 0) != 0) {
            continue;
        }
        const auto pos = line.find(':');
        if (pos == std::string::npos) {
            continue;
        }
        std::istringstream iss(line.substr(pos + 1));
        double freq_mhz = 0.0;
        if (iss >> freq_mhz) {
            return freq_mhz;
        }
    }
    return 0.0;
}

}  // namespace web_htop::server::collectors
