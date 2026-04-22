/**
 * @file server/include/server/collectors/network_collector.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-17
 *
 * @brief Collects aggregate network counters and rates from /proc/net/dev.
 */

#pragma once

#include "common/models/network_metrics.hpp"

#include <cstdint>
#include <optional>

namespace web_htop::server::collectors {

class NetworkCollector {
public:
    NetworkCollector() = default;

    /** @brief First sample returns 0 rates; later calls compute delta / elapsed time. */
    [[nodiscard]] web_htop::models::NetworkMetrics Collect();

private:
    struct Totals {
        std::uint64_t rx_bytes = 0;
        std::uint64_t tx_bytes = 0;
    };

    [[nodiscard]] Totals ReadProcNetDev() const;
    [[nodiscard]] std::uint64_t CurrentTimestamp() const;

    std::optional<Totals> previous_totals_{};
    std::optional<std::uint64_t> previous_time_ms_{};
};

}  // namespace web_htop::server::collectors
