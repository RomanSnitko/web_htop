/**
 * @file common/models/cpu_metrics.hpp
 *
 * @author Maksim Vashkevich
 * @date 2026-04-07
 *
 * @brief CPU metrics model for one collection snapshot.
 * @details This header defines CPU-related metrics exchanged between
 * collectors and protocol messages, including serialization helpers.
 */

#ifndef WEB_HTOP_COMMON_MODELS_CPU_METRICS_HPP_
#define WEB_HTOP_COMMON_MODELS_CPU_METRICS_HPP_

#include <cstdint> // std::uint64_t for jiffies counters
#include <vector>  // std::vector for per-core usage

#include "common/json/utils.hpp"
#include "common/types.hpp"

namespace web_htop::models {

/**
 * @brief CPU metrics for a single sampling point.
 * @details Contains both user-facing values and raw jiffies used by collector
 * to calculate percentages between consecutive snapshots. JSON
 * serialization/deserialization includes fields: `timestamp`, `core_count`,
 * `total_usage_percent`, `frequency_mhz`, `per_core_usage_percent`.
 */
struct CPUMetrics {
    TimeStamp               timestamp{};              ///< Sampling time in milliseconds since epoch
    CPUCores                core_count{};             ///< Number of currently online CPU cores
    Percentage              total_usage_percent{};    ///< Aggregated CPU usage in range [0.0, 100.0]
    std::vector<Percentage> per_core_usage_percent{}; ///< Per-core CPU usage in range [0.0, 100.0]
    MegaHertz               frequency_mhz{};          ///< Current CPU frequency in MHz

    /**
     * @brief Serialize CPU metrics to JSON object.
     * @returns JSON object containing public CPU metrics fields.
     */
    [[nodiscard]] json::utils::JSONValue ToJson() const;

    /**
     * @brief Restore CPU metrics from JSON object.
     * @param value JSON value expected to be object.
     * @returns Parsed `CPUMetrics` with absent fields left as defaults.
     */
    [[nodiscard]] static CPUMetrics FromJson(json::utils::JSONValue const & value);
};

} // namespace web_htop::models

#endif // WEB_HTOP_COMMON_MODELS_CPU_METRICS_HPP_
