/**
 * @file common/types.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-03
 *
 * @brief Common type definitions for web_htop project
 * @details This header defines basic type aliases for timestamps,
 * percentages, frequencies, and resource metrics.
 */

#pragma once

#include <cstdint>     // Fixed-width integer types (std::uint64_t, std::uint32_t)
#include <sys/types.h> // pid_t for process IDs

namespace web_htop
{

// Basic type aliases for clarity and consistency
using TimeStamp = std::uint64_t; ///< Milliseconds since epoch
using Percentage = double;       ///< Percentage, [0.0, 100.0]
using MegaHertz = double;        ///< MHz, commonly used for CPU frequency

// Aliases for resource/network metrics
using Bytes = std::uint64_t;       ///< Bytes, commonly used for memory/disk size
using KilobytesPerSecond = double; ///< KiB/s; retained for the v1 metric fields

// Aliases for system metrics
using CPUCores = std::uint32_t; ///< Number of CPU cores
using ProcessID = pid_t;        ///< PID type

} // namespace web_htop
