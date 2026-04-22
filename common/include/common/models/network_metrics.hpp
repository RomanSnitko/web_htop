/**
 * @file common/models/network_metrics.hpp
 *
 * @author Maksim Vashkevich
 * @date 2026-04-18
 *
 * @brief Network metrics model for one collection snapshot.
 * @details Describes aggregate network traffic counters and rates, and
 * provides JSON conversion helpers.
 */

#ifndef WEB_HTOP_COMMON_MODELS_NETWORK_METRICS_HPP_
#define WEB_HTOP_COMMON_MODELS_NETWORK_METRICS_HPP_

#include "common/json/utils.hpp"
#include "common/types.hpp"

namespace web_htop::models {

/**
 * @brief Network subsystem metrics for one sampling point.
 * @details JSON serialization/deserialization includes fields: `timestamp`,
 * `rx_bytes_total`, `tx_bytes_total`, `rx_kbps`, `tx_kbps`.
 */
struct NetworkMetrics {
    TimeStamp          timestamp{};      ///< Sampling time in milliseconds since epoch
    Bytes              rx_bytes_total{}; ///< Total received bytes since boot
    Bytes              tx_bytes_total{}; ///< Total transmitted bytes since boot
    KilobytesPerSecond rx_kbps{};        ///< Receive throughput in KB/s
    KilobytesPerSecond tx_kbps{};        ///< Transmit throughput in KB/s

    /**
     * @brief Serialize network metrics to JSON object.
     * @returns JSON object containing network metrics fields.
     */
    [[nodiscard]] json::utils::JSONValue ToJson() const;

    /**
     * @brief Restore network metrics from JSON object.
     * @param value JSON value expected to be object.
     * @returns Parsed `NetworkMetrics` with absent fields left as defaults.
     */
    [[nodiscard]] static NetworkMetrics FromJson(json::utils::JSONValue const & value);
};

} // namespace web_htop::models

#endif // WEB_HTOP_COMMON_MODELS_NETWORK_METRICS_HPP_
