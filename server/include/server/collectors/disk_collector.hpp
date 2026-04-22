/**
 * @file server/include/server/collectors/disk_collector.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-17
 *
 * @brief Collects disk usage for a filesystem path (default: root).
 */

#pragma once

#include "common/models/disk_metrics.hpp"

#include <string>

namespace web_htop::server::collectors {

class DiskCollector {
public:
    explicit DiskCollector(std::string mount_path = "/");

    [[nodiscard]] web_htop::models::DiskMetrics Collect();

private:
    [[nodiscard]] std::uint64_t CurrentTimestamp() const;

    std::string mount_path_;
};

}  // namespace web_htop::server::collectors
