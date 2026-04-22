/**
 * @file common/models/system_snapshot.hpp
 *
 * @author Maxim Vashkevich
 * @date 2026-04-20
 *
 * @brief Aggregated system snapshot model.
 * @details Combines per-subsystem metrics into one transport-friendly
 * structure with JSON conversion helpers.
 */

#ifndef WEB_HTOP_COMMON_MODELS_SYSTEM_SNAPSHOT_HPP_
#define WEB_HTOP_COMMON_MODELS_SYSTEM_SNAPSHOT_HPP_

#include "common/json/utils.hpp"
#include "common/models/cpu_metrics.hpp"
#include "common/models/disk_metrics.hpp"
#include "common/models/loadavg_metrics.hpp"
#include "common/models/memory_metrics.hpp"
#include "common/models/network_metrics.hpp"
#include "common/models/process_metrics.hpp"
#include "common/types.hpp"

namespace web_htop::models {

/**
 * @brief Full system metrics snapshot for one sampling point.
 * @details JSON serialization/deserialization includes fields: `timestamp`,
 * `cpu`, `memory`, `disk`, `network`, `loadavg`, `process`.
 */
struct SystemSnapshot {
    TimeStamp      timestamp{}; ///< Snapshot timestamp in milliseconds since epoch
    CPUMetrics     cpu{};       ///< CPU metrics section
    MemoryMetrics  memory{};    ///< Memory metrics section
    DiskMetrics    disk{};      ///< Disk metrics section
    NetworkMetrics network{};   ///< Network metrics section
    LoadavgMetrics loadavg{};   ///< Load average metrics section
    ProcessMetrics process{};   ///< Process metrics section

    /**
     * @brief Serialize full system snapshot to JSON object.
     * @returns JSON object containing all system snapshot sections.
     */
    [[nodiscard]] json::utils::JSONValue ToJson() const;

    /**
     * @brief Restore full system snapshot from JSON object.
     * @param value JSON value expected to be object.
     * @returns Parsed `SystemSnapshot` with absent fields left as defaults.
     */
    [[nodiscard]] static SystemSnapshot FromJson(json::utils::JSONValue const & value);
};

} // namespace web_htop::models

#endif // WEB_HTOP_COMMON_MODELS_SYSTEM_SNAPSHOT_HPP_
