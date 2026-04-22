/**
 * @file common/models/disk_metrics.hpp
 *
 * @author Maksim Vashkevich
 * @date 2026-04-18
 *
 * @brief Disk metrics model for one collection snapshot.
 * @details Describes filesystem capacity and usage values and provides JSON
 * conversion helpers.
 */

#ifndef WEB_HTOP_COMMON_MODELS_DISK_METRICS_HPP_
#define WEB_HTOP_COMMON_MODELS_DISK_METRICS_HPP_

#include "common/json/utils.hpp"
#include "common/types.hpp"

namespace web_htop::models {

/**
 * @brief Disk subsystem metrics for one sampling point.
 * @details JSON serialization/deserialization includes fields: `timestamp`,
 * `total_bytes`, `available_bytes`, `used_bytes`, `used_percent`.
 */
struct DiskMetrics {
    TimeStamp  timestamp{};       ///< Sampling time in milliseconds since epoch
    Bytes      total_bytes{};     ///< Total filesystem size in bytes
    Bytes      available_bytes{}; ///< Space available for non-privileged users
    Bytes      used_bytes{};      ///< Computed used size (`total - available`)
    Percentage used_percent{};    ///< Used space in range [0.0, 100.0]

    /**
     * @brief Serialize disk metrics to JSON object.
     * @returns JSON object containing disk metrics fields.
     */
    [[nodiscard]] json::utils::JSONValue ToJson() const;

    /**
     * @brief Restore disk metrics from JSON object.
     * @param value JSON value expected to be object.
     * @returns Parsed `DiskMetrics` with absent fields left as defaults.
     */
    [[nodiscard]] static DiskMetrics FromJson(json::utils::JSONValue const & value);
};

} // namespace web_htop::models

#endif // WEB_HTOP_COMMON_MODELS_DISK_METRICS_HPP_