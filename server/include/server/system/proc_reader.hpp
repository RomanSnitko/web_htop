/**
 * @file server/include/server/system/proc_reader.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-16
 *
 * @brief Linux /proc reader helpers used by server collectors.
 */

#pragma once

#include "common/enums.hpp"
#include "common/types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace web_htop::server::system {

class ProcReader {
public:
    struct ProcessSample {
        ProcessID pid{};
        std::string name{};
        ProcessState state{ProcessState::UNKNOWN};
        std::uint64_t cpu_ticks{};
        Bytes memory_bytes{};
        CPUCores thread_count{};
    };

    [[nodiscard]] std::vector<ProcessSample> ReadProcesses() const;
    [[nodiscard]] std::uint64_t ReadSystemCpuTicks() const;

private:
    struct StatSample {
        ProcessState state{ProcessState::UNKNOWN};
        std::uint64_t cpu_ticks{};
    };

    [[nodiscard]] std::vector<ProcessID> EnumeratePids() const;
    [[nodiscard]] std::optional<std::string> ReadComm(ProcessID pid) const;
    [[nodiscard]] std::optional<StatSample> ReadStat(ProcessID pid) const;
    [[nodiscard]] std::optional<std::pair<Bytes, CPUCores>> ReadStatus(ProcessID pid) const;
};

}  // namespace web_htop::server::system
