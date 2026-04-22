/**
 * @file server/include/server/collectors/loadavg_collector.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-17
 *
 * @brief Collects load average from /proc/loadavg.
 */

#pragma once

#include "common/models/loadavg_metrics.hpp"

namespace web_htop::server::collectors {

class LoadavgCollector {
public:
    LoadavgCollector() = default;

    [[nodiscard]] web_htop::models::LoadavgMetrics Collect();

private:
    [[nodiscard]] std::uint64_t CurrentTimestamp() const;
};

}  // namespace web_htop::server::collectors
