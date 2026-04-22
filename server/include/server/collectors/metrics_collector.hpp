/**
 * @file server/include/server/collectors/metrics_collector.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-17
 *
 * @brief Runs sub-collectors and builds one SystemSnapshot.
 */

#pragma once

#include "common/models/system_snapshot.hpp"
#include "server/collectors/cpu_collector.hpp"
#include "server/collectors/disk_collector.hpp"
#include "server/collectors/loadavg_collector.hpp"
#include "server/collectors/memory_collector.hpp"
#include "server/collectors/network_collector.hpp"
#include "server/collectors/process_collector.hpp"

namespace web_htop::server::collectors {

class MetricsCollector {
public:
    MetricsCollector() = default;

    [[nodiscard]] web_htop::models::SystemSnapshot Collect();

private:
    CpuCollector cpu_collector_{};
    MemoryCollector memory_collector_{};
    DiskCollector disk_collector_{};
    NetworkCollector network_collector_{};
    LoadavgCollector loadavg_collector_{};
    ProcessCollector process_collector_{};
};

}  // namespace web_htop::server::collectors
