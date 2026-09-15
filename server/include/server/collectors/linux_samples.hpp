/** @file server/collectors/linux_samples.hpp
 *  @brief Pure parsing and counter arithmetic, independent of the running host.
 */
#pragma once
#include "common/telemetry.hpp"
#include <array>
#include <map>
#include <optional>
#include <string_view>

namespace web_htop::server::collectors
{
using CpuTimes = std::array<std::uint64_t, 8>;
using CpuSample = std::map<int, CpuTimes>; // -1 is the aggregate; other keys are Linux CPU IDs.

struct ProcessSample
{
    int pid{};
    int ppid{};
    std::string name;
    char state{};
    std::uint64_t ticks{}, starttime{}, rss_pages{};
    std::uint32_t threads{};
};

[[nodiscard]] CpuSample ParseCpu(std::string_view text);
[[nodiscard]] std::optional<double> CpuUsage(CpuTimes const& now, CpuTimes const& previous);
[[nodiscard]] std::optional<ProcessSample> ParseProcess(std::string_view text);
[[nodiscard]] std::optional<double> CounterRate(std::uint64_t now, std::uint64_t previous,
                                                double seconds);
[[nodiscard]] models::PressureMetrics ParsePressure(std::string resource, std::string_view text);
} // namespace web_htop::server::collectors
