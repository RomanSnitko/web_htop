/**
 * @file server/src/collectors/memory_collector.cpp
 *
 * @author Roman Snitko
 * @date 2026-04-17
 *
 * @brief MemoryCollector implementation.
 */

#include "server/collectors/memory_collector.hpp"

#include <chrono>
#include <fstream>
#include <sstream>

namespace web_htop::server::collectors {

namespace {

constexpr std::uint64_t kBytesPerKib = 1024;

}  // namespace

web_htop::models::MemoryMetrics MemoryCollector::Collect() {
    const MemSnapshot snap = ReadProcMeminfo();

    web_htop::models::MemoryMetrics metrics{};
    metrics.timestamp = CurrentTimestamp();
    metrics.total_bytes = snap.mem_total_kb * kBytesPerKib;
    metrics.available_bytes = snap.mem_available_kb * kBytesPerKib;
    if (metrics.total_bytes >= metrics.available_bytes) {
        metrics.used_bytes = metrics.total_bytes - metrics.available_bytes;
    } else {
        metrics.used_bytes = 0;
    }
    if (metrics.total_bytes == 0) {
        metrics.used_percent = 0.0;
    } else {
        metrics.used_percent =
            (static_cast<double>(metrics.used_bytes) / static_cast<double>(metrics.total_bytes)) *
            100.0;
    }
    return metrics;
}

std::uint64_t MemoryCollector::CurrentTimestamp() const {
    const auto now = std::chrono::system_clock::now();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

bool MemoryCollector::ParseMeminfoLine(const std::string& line, std::string& key,
                                         std::uint64_t& value_kb) const {
    std::istringstream iss(line);
    std::string key_with_colon;
    std::string unit;

    if (!(iss >> key_with_colon >> value_kb)) {
        return false;
    }

    if (!key_with_colon.empty() && key_with_colon.back() == ':') {
        key = key_with_colon.substr(0, key_with_colon.size() - 1);
    } else {
        key = key_with_colon;
    }

    iss >> unit;
    (void)unit;
    return true;
}

MemoryCollector::MemSnapshot MemoryCollector::ReadProcMeminfo() const {
    std::ifstream file("/proc/meminfo");
    std::string line;
    MemSnapshot snapshot{};

    while (std::getline(file, line)) {
        std::string key;
        std::uint64_t value_kb = 0;
        if (!ParseMeminfoLine(line, key, value_kb)) {
            continue;
        }
        if (key == "MemTotal") {
            snapshot.mem_total_kb = value_kb;
        } else if (key == "MemAvailable") {
            snapshot.mem_available_kb = value_kb;
        }
    }

    return snapshot;
}

}  // namespace web_htop::server::collectors
