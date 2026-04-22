/**
 * @file common/models/loadavg_metrics.hpp
 *
 * @author Maxim Vashkevich
 * @date 2026-04-20
 *
 * @brief Load average metrics model for one collection snapshot.
 * @details Describes Linux load averages for 1, 5 and 15 minutes and
 * provides JSON conversion helpers.
 */

#ifndef WEB_HTOP_COMMON_MODELS_LOADAVG_METRICS_HPP_
#define WEB_HTOP_COMMON_MODELS_LOADAVG_METRICS_HPP_

#include "common/json/utils.hpp"
#include "common/types.hpp"

namespace web_htop::models {

/**
 * @brief System load average metrics for one sampling point.
 * @details JSON serialization/deserialization includes fields: `timestamp`,
 * `load_1m`, `load_5m`, `load_15m`.
 */
struct LoadavgMetrics {
    TimeStamp  timestamp{}; ///< Sampling time in milliseconds since epoch
    Percentage load_1m{};   ///< Average runnable entities in last 1 minute
    Percentage load_5m{};   ///< Average runnable entities in last 5 minutes
    Percentage load_15m{};  ///< Average runnable entities in last 15 minutes

    /**
     * @brief Serialize load average metrics to JSON object.
     * @returns JSON object containing load average metrics fields.
     */
    [[nodiscard]] json::utils::JSONValue ToJson() const;

    /**
     * @brief Restore load average metrics from JSON object.
     * @param value JSON value expected to be object.
     * @returns Parsed `LoadavgMetrics` with absent fields left as defaults.
     */
    [[nodiscard]] static LoadavgMetrics FromJson(json::utils::JSONValue const & value);
};

} // namespace web_htop::models

#endif // WEB_HTOP_COMMON_MODELS_LOADAVG_METRICS_HPP_
