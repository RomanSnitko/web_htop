/**
 * @file server/include/server/collectors/cpu_collector.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-17
 *
 * @brief Collects CPU metrics from /proc/stat and /proc/cpuinfo.
 */

#pragma once

#include "common/models/cpu_metrics.hpp"

#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace web_htop::server::collectors {

class CpuCollector {
public:
    CpuCollector() = default;

    /** @brief Samples CPU usage; first call may return 0% usage until baseline exists. */
    [[nodiscard]] web_htop::models::CPUMetrics Collect();

private:
    struct CpuTimes {
        std::uint64_t user = 0;
        std::uint64_t nice = 0;
        std::uint64_t system = 0;
        std::uint64_t idle = 0;
        std::uint64_t iowait = 0;
        std::uint64_t irq = 0;
        std::uint64_t softirq = 0;
        std::uint64_t steal = 0;

        [[nodiscard]] std::uint64_t Total() const;
        [[nodiscard]] std::uint64_t IdleTotal() const;
    };

    struct CpuSnapshot {
        CpuTimes total{};
        std::vector<CpuTimes> per_core{};
    };

    [[nodiscard]] CpuTimes ParseCpuLine(const std::string& line) const;
    [[nodiscard]] CpuSnapshot ReadProcStatInternal() const;
    [[nodiscard]] CpuSnapshot ReadProcStat() const;

    [[nodiscard]] double CalculateUsage(CpuTimes const& current, CpuTimes const& previous) const;
    [[nodiscard]] double ReadFrequencyMHz() const;
    [[nodiscard]] std::uint64_t CurrentTimestamp() const;

    std::optional<CpuSnapshot> previous_snapshot_{};
};

}  // namespace web_htop::server::collectors
