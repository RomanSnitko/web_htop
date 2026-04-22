/**
 * @file server/src/collectors/disk_collector.cpp
 *
 * @author Roman Snitko
 * @date 2026-04-17
 *
 * @brief DiskCollector implementation (statvfs).
 */

#include "server/collectors/disk_collector.hpp"

#include <chrono>
#include <sys/statvfs.h>

namespace web_htop::server::collectors {

DiskCollector::DiskCollector(std::string mount_path) : mount_path_(std::move(mount_path)) {}

web_htop::models::DiskMetrics DiskCollector::Collect() {
    struct statvfs st {};
    if (statvfs(mount_path_.c_str(), &st) != 0) {
        return {};
    }

    const std::uint64_t block_size = st.f_frsize;
    const std::uint64_t total_bytes = st.f_blocks * block_size;
    const std::uint64_t available_bytes = st.f_bavail * block_size;

    web_htop::models::DiskMetrics metrics{};
    metrics.timestamp = CurrentTimestamp();
    metrics.total_bytes = total_bytes;
    metrics.available_bytes = available_bytes;
    if (total_bytes >= available_bytes) {
        metrics.used_bytes = total_bytes - available_bytes;
    } else {
        metrics.used_bytes = 0;
    }
    if (total_bytes == 0) {
        metrics.used_percent = 0.0;
    } else {
        metrics.used_percent =
            (static_cast<double>(metrics.used_bytes) / static_cast<double>(total_bytes)) * 100.0;
    }
    return metrics;
}

std::uint64_t DiskCollector::CurrentTimestamp() const {
    const auto now = std::chrono::system_clock::now();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

}  // namespace web_htop::server::collectors
