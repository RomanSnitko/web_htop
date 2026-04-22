/**
 * @file server/src/collectors/process_collector.cpp
 *
 * @author Roman Snitko
 * @date 2026-04-16
 *
 * @brief ProcessCollector implementation.
 */

#include "server/collectors/process_collector.hpp"

#include <algorithm>
#include <chrono>
#include <unistd.h>

namespace web_htop::server::collectors {

namespace {

[[nodiscard]] double ClampPercent(double value) {
    if (value < 0.0) {
        return 0.0;
    }
    if (value > 100.0) {
        return 100.0;
    }
    return value;
}

}  // namespace

web_htop::models::ProcessMetrics ProcessCollector::Collect() {
    const auto samples = proc_reader_.ReadProcesses();
    const std::uint64_t system_ticks = proc_reader_.ReadSystemCpuTicks();
    const std::uint64_t system_ticks_delta =
        previous_system_ticks_ == 0 || system_ticks < previous_system_ticks_
            ? 0
            : system_ticks - previous_system_ticks_;
    const Bytes total_memory_bytes = ReadTotalMemoryBytes();

    web_htop::models::ProcessMetrics metrics{};
    metrics.timestamp = CurrentTimestamp();
    metrics.total_processes = samples.size();
    metrics.processes.reserve(samples.size());

    std::unordered_map<ProcessID, std::uint64_t> current_ticks{};
    current_ticks.reserve(samples.size());

    for (auto const& sample : samples) {
        web_htop::models::ProcessInfo info{};
        info.pid = sample.pid;
        info.name = sample.name;
        info.state = sample.state;
        info.memory_bytes = sample.memory_bytes;
        info.thread_count = sample.thread_count;

        if (sample.state == ProcessState::RUNNING) {
            ++metrics.running_processes;
        }

        const auto previous_it = previous_process_ticks_.find(sample.pid);
        if (system_ticks_delta != 0 && previous_it != previous_process_ticks_.end() &&
            sample.cpu_ticks >= previous_it->second) {
            info.cpu_percent =
                CalculateCpuPercent(sample.cpu_ticks, previous_it->second, system_ticks_delta);
        }

        if (total_memory_bytes != 0) {
            info.memory_percent =
                ClampPercent((static_cast<double>(sample.memory_bytes) /
                              static_cast<double>(total_memory_bytes)) *
                             100.0);
        }

        current_ticks[sample.pid] = sample.cpu_ticks;
        metrics.processes.push_back(std::move(info));
    }

    std::sort(metrics.processes.begin(), metrics.processes.end(),
              [](web_htop::models::ProcessInfo const& lhs,
                 web_htop::models::ProcessInfo const& rhs) {
                  if (lhs.cpu_percent != rhs.cpu_percent) {
                      return lhs.cpu_percent > rhs.cpu_percent;
                  }
                  if (lhs.memory_bytes != rhs.memory_bytes) {
                      return lhs.memory_bytes > rhs.memory_bytes;
                  }
                  return lhs.pid < rhs.pid;
              });

    previous_process_ticks_ = std::move(current_ticks);
    previous_system_ticks_ = system_ticks;
    return metrics;
}

std::uint64_t ProcessCollector::CurrentTimestamp() const {
    const auto now = std::chrono::system_clock::now();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

Bytes ProcessCollector::ReadTotalMemoryBytes() const {
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages <= 0 || page_size <= 0) {
        return 0;
    }
    return static_cast<Bytes>(pages) * static_cast<Bytes>(page_size);
}

double ProcessCollector::CalculateCpuPercent(std::uint64_t current_ticks,
                                             std::uint64_t previous_ticks,
                                             std::uint64_t system_ticks_delta) const {
    if (system_ticks_delta == 0 || current_ticks < previous_ticks) {
        return 0.0;
    }

    const std::uint64_t process_ticks_delta = current_ticks - previous_ticks;
    return ClampPercent((static_cast<double>(process_ticks_delta) /
                         static_cast<double>(system_ticks_delta)) *
                        100.0);
}

}  // namespace web_htop::server::collectors
