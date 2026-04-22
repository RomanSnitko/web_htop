/**
 * @file common/types.hpp
 *
 * @author Maksim Vashkevich
 * @date 2026-04-03
 * 
 * @brief Common type definitions for web_htop project
 * @details This header defines basic type aliases for timestamps,
 * percentages, frequencies, and resource metrics.
 */

#ifndef WEB_HTOP_COMMON_TYPES_HPP_
#define WEB_HTOP_COMMON_TYPES_HPP_

#include <cstdint>     // Fixed-width integer types (std::uint64_t, std::uint16_t)
#include <sys/types.h> // pid_t for process IDs

namespace web_htop {

// Basic type aliases for clarity and consistency
using TimeStamp  = std::uint64_t; ///< Milliseconds since epoch
using Percentage = double;        ///< Percentage, [0.0, 100.0]
using MegaHertz  = double;        ///< MHz, commonly used for CPU frequency

// Aliases for resource/network metrics
using Bytes              = std::uint64_t; ///< Bytes, commonly used for memory/disk size
using KilobytesPerSecond = double;        ///< Kbps, commonly used for network speed

// Aliases for system metrics
using CPUCores  = std::uint16_t; ///< Number of CPU cores
using ProcessID = pid_t;         ///< PID type

} // namespace web_htop

#endif // WEB_HTOP_COMMON_TYPES_HPP_