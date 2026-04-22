/**
 * @file server/include/server/collectors/process_collector.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-16
 *
 * @brief Collects process table metrics from `/proc`.
 */

#pragma once

#include "common/models/process_metrics.hpp"
#include "server/system/proc_reader.hpp"

#include <cstdint>
#include <unordered_map>

namespace web_htop::server::collectors {

class ProcessCollector {
public:
    ProcessCollector() = default;

    [[nodiscard]] web_htop::models::ProcessMetrics Collect();

private:
    [[nodiscard]] std::uint64_t CurrentTimestamp() const;
    [[nodiscard]] Bytes ReadTotalMemoryBytes() const;
    [[nodiscard]] double CalculateCpuPercent(std::uint64_t current_ticks,
                                             std::uint64_t previous_ticks,
                                             std::uint64_t system_ticks_delta) const;

    system::ProcReader proc_reader_{};
    std::unordered_map<ProcessID, std::uint64_t> previous_process_ticks_{};
    std::uint64_t previous_system_ticks_{0};
};

}  // namespace web_htop::server::collectors
