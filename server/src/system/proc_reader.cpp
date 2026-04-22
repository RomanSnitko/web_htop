/**
 * @file server/src/system/proc_reader.cpp
 *
 * @author Roman Snitko
 * @date 2026-04-16
 *
 * @brief /proc reading implementation for process collector.
 */

#include "server/system/proc_reader.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace web_htop::server::system {

namespace {

constexpr std::uint64_t kBytesPerKib = 1024;

[[nodiscard]] bool IsNumericName(std::string const& value) {
    return !value.empty() &&
           std::all_of(value.begin(), value.end(), [](unsigned char ch) { return std::isdigit(ch) != 0; });
}

}  // namespace

std::vector<ProcessID> ProcReader::EnumeratePids() const {
    std::vector<ProcessID> pids{};

    try {
        if (!std::filesystem::exists("/proc")) {
            return pids;
        }

        for (auto const& entry : std::filesystem::directory_iterator("/proc")) {
            if (!entry.is_directory()) {
                continue;
            }

            const std::string name = entry.path().filename().string();
            if (!IsNumericName(name)) {
                continue;
            }

            try {
                pids.push_back(static_cast<ProcessID>(std::stoi(name)));
            } catch (...) {
                continue;
            }
        }
    } catch (...) {
        return {};
    }

    std::sort(pids.begin(), pids.end());
    return pids;
}

std::optional<std::string> ProcReader::ReadComm(ProcessID pid) const {
    std::ifstream file("/proc/" + std::to_string(pid) + "/comm");
    std::string name;
    if (!std::getline(file, name)) {
        return std::nullopt;
    }
    return name;
}

std::optional<ProcReader::StatSample> ProcReader::ReadStat(ProcessID pid) const {
    std::ifstream file("/proc/" + std::to_string(pid) + "/stat");
    std::string line;
    if (!std::getline(file, line)) {
        return std::nullopt;
    }

    const std::size_t close_pos = line.rfind(')');
    if (close_pos == std::string::npos || close_pos + 2 >= line.size()) {
        return std::nullopt;
    }

    std::istringstream iss(line.substr(close_pos + 2));
    char state_char = '?';
    if (!(iss >> state_char)) {
        return std::nullopt;
    }

    std::vector<std::string> fields{};
    std::string token;
    while (iss >> token) {
        fields.push_back(token);
    }

    if (fields.size() <= 11) {
        return std::nullopt;
    }

    StatSample sample{};
    sample.state = static_cast<ProcessState>(state_char);

    try {
        const std::uint64_t utime = std::stoull(fields[10]);
        const std::uint64_t stime = std::stoull(fields[11]);
        sample.cpu_ticks = utime + stime;
    } catch (...) {
        return std::nullopt;
    }

    return sample;
}

std::optional<std::pair<Bytes, CPUCores>> ProcReader::ReadStatus(ProcessID pid) const {
    std::ifstream file("/proc/" + std::to_string(pid) + "/status");
    std::string line;
    Bytes memory_bytes = 0;
    CPUCores thread_count = 0;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key)) {
            continue;
        }

        if (key == "VmRSS:") {
            std::uint64_t value_kb = 0;
            if (iss >> value_kb) {
                memory_bytes = value_kb * kBytesPerKib;
            }
        } else if (key == "Threads:") {
            unsigned long value = 0;
            if (iss >> value) {
                thread_count = static_cast<CPUCores>(value);
            }
        }
    }

    if (!file.eof() && file.fail()) {
        return std::nullopt;
    }

    return std::make_pair(memory_bytes, thread_count);
}

std::vector<ProcReader::ProcessSample> ProcReader::ReadProcesses() const {
    std::vector<ProcessSample> processes{};
    const auto pids = EnumeratePids();
    processes.reserve(pids.size());

    for (ProcessID pid : pids) {
        const auto name = ReadComm(pid);
        const auto stat = ReadStat(pid);
        const auto status = ReadStatus(pid);
        if (!name || !stat || !status) {
            continue;
        }

        ProcessSample sample{};
        sample.pid = pid;
        sample.name = *name;
        sample.state = stat->state;
        sample.cpu_ticks = stat->cpu_ticks;
        sample.memory_bytes = status->first;
        sample.thread_count = status->second;
        processes.push_back(std::move(sample));
    }

    return processes;
}

std::uint64_t ProcReader::ReadSystemCpuTicks() const {
    std::ifstream file("/proc/stat");
    std::string label;
    if (!(file >> label) || label != "cpu") {
        return 0;
    }

    std::uint64_t ticks = 0;
    std::uint64_t value = 0;
    while (file >> value) {
        ticks += value;
        if (file.peek() == '\n') {
            break;
        }
    }

    return ticks;
}

}  // namespace web_htop::server::system
