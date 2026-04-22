/**
 * @file common/models/memory_metrics.hpp
 *
 * @author Maksim Vashkevich
 * @date 2026-04-08
 *
 * @brief Memory metrics model for one collection snapshot.
 * @details Describes RAM totals and usage values collected from `/proc/meminfo`
 * and provides JSON conversion helpers.
 */

#ifndef WEB_HTOP_COMMON_MODELS_MEMORY_METRICS_HPP_
#define WEB_HTOP_COMMON_MODELS_MEMORY_METRICS_HPP_
 
#include "common/json/utils.hpp"
#include "common/types.hpp"
 
namespace web_htop::models {
 
/**
 * @brief Memory subsystem metrics for one sampling point.
 * @details JSON serialization/deserialization includes fields: `timestamp`,
 * `total_bytes`, `available_bytes`, `used_bytes`, `used_percent`.
 */
struct MemoryMetrics {
    TimeStamp  timestamp{};       ///< Sampling time in milliseconds since epoch
    Bytes      total_bytes{};     ///< Total available system memory
    Bytes      available_bytes{}; ///< Readily available memory for new processes
    Bytes      used_bytes{};      ///< Computed used memory (`total - available`)
    Percentage used_percent{};    ///< Used memory in range [0.0, 100.0]
 
    /**
     * @brief Serialize memory metrics to JSON object.
     * @returns JSON object containing memory metrics fields.
     */
    [[nodiscard]] json::utils::JSONValue ToJson() const;
 
    /**
     * @brief Restore memory metrics from JSON object.
     * @param value JSON value expected to be object.
     * @returns Parsed `MemoryMetrics` with absent fields left as defaults.
     */
    [[nodiscard]] static MemoryMetrics FromJson(json::utils::JSONValue const & value);
};
 
} // namespace web_htop::models
 
#endif // WEB_HTOP_COMMON_MODELS_MEMORY_METRICS_HPP_