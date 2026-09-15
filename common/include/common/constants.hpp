/**
 * @file common/constants.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-06
 *
 * @brief Common project constants for web_htop.
 * @details This header defines constants shared across modules.
 * It currently contains Linux `/proc` filesystem paths used by collectors.
 */

#pragma once

#include <string_view> // std::string_view for constant string paths

namespace web_htop
{

/**
 * @namespace /proc filesystem paths
 *
 * @brief Constants for paths within the /proc filesystem
 */
namespace proc
{

constexpr std::string_view PROC = "/proc/";     ///< `/proc` prefix for process-specific file paths
constexpr std::string_view STAT = "/proc/stat"; ///< Path to aggregated CPU statistics
constexpr std::string_view MEMINFO = "/proc/meminfo"; ///< Path to memory subsystem information
constexpr std::string_view NET_DEV = "/proc/net/dev"; ///< Path to network device statistics
constexpr std::string_view LOADAVG = "/proc/loadavg"; ///< Path to system load averages
constexpr std::string_view UPTIME = "/proc/uptime";   ///< Path to system uptime information

} // namespace proc

} // namespace web_htop
