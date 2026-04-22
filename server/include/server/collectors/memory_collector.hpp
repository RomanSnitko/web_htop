/**
 * @file server/include/server/collectors/memory_collector.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-17
 *
 * @brief Collects memory metrics from /proc/meminfo.
 */

#pragma once

#include "common/models/memory_metrics.hpp"

#include <cstdint>
#include <string>

namespace web_htop::server::collectors {

class MemoryCollector {
public:
    MemoryCollector() = default;

    [[nodiscard]] web_htop::models::MemoryMetrics Collect();

private:
    struct MemSnapshot {
        std::uint64_t mem_total_kb = 0;
        std::uint64_t mem_available_kb = 0;
    };

    [[nodiscard]] MemSnapshot ReadProcMeminfo() const;
    [[nodiscard]] bool ParseMeminfoLine(const std::string& line, std::string& key,
                                         std::uint64_t& value_kb) const;
    [[nodiscard]] std::uint64_t CurrentTimestamp() const;
};

}  // namespace web_htop::server::collectors
