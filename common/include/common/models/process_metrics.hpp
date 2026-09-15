/**
 * @file common/models/process_metrics.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-18
 *
 * @brief Process list metrics models for one collection snapshot.
 * @details Defines per-process model and process list snapshot model with JSON
 * conversion helpers.
 */

#pragma once

#include <cstdint> // std::uint64_t
#include <string>  // std::string
#include <vector>  // std::vector

#include "common/enums.hpp"
#include "common/json/utils.hpp"
#include "common/types.hpp"

namespace web_htop::models
{

/**
 * @brief One process entry for the process table snapshot.
 * @details JSON serialization/deserialization includes fields: `pid`,
 * `name`, `state`, `cpu_percent`, `memory_bytes`, `memory_percent`,
 * `thread_count`.
 */
struct ProcessInfo
{
    std::uint64_t starttime_ticks{};
    bool cpu_valid{};
    ProcessID pid{};                           ///< Process identifier
    ProcessID ppid{};                          ///< Parent process identifier
    std::string name{};                        ///< Process name (comm)
    ProcessState state{ProcessState::UNKNOWN}; ///< Process state from `/proc/[pid]/stat`
    Percentage cpu_percent{};                  ///< CPU usage: one fully busy core is 100%
    Bytes memory_bytes{};                      ///< Resident memory usage in bytes
    Percentage memory_percent{};               ///< Process memory usage in range [0.0, 100.0]
    CPUCores thread_count{};                   ///< Number of process threads

    /**
     * @brief Serialize process info to JSON object.
     * @returns JSON object containing process fields.
     */
    [[nodiscard]] json::utils::JSONValue ToJson() const;

    /**
     * @brief Restore process info from JSON object.
     * @param value JSON value expected to be object.
     * @returns Parsed `ProcessInfo` with absent fields left as defaults.
     */
    [[nodiscard]] static ProcessInfo FromJson(json::utils::JSONValue const& value);
};

/**
 * @brief Process list metrics for one sampling point.
 * @details JSON serialization/deserialization includes fields: `timestamp`,
 * `total_processes`, `running_processes`, `processes`.
 */
struct ProcessMetrics
{
    TimeStamp timestamp{};                ///< Sampling time in milliseconds since epoch
    std::uint64_t total_processes{};      ///< Total number of collected processes
    std::uint64_t running_processes{};    ///< Number of processes in running state
    std::vector<ProcessInfo> processes{}; ///< Process list snapshot

    /**
     * @brief Serialize process list metrics to JSON object.
     * @returns JSON object containing process list metrics fields.
     */
    [[nodiscard]] json::utils::JSONValue ToJson() const;

    /**
     * @brief Restore process list metrics from JSON object.
     * @param value JSON value expected to be object.
     * @returns Parsed `ProcessMetrics` with absent fields left as defaults.
     */
    [[nodiscard]] static ProcessMetrics FromJson(json::utils::JSONValue const& value);
};

} // namespace web_htop::models
