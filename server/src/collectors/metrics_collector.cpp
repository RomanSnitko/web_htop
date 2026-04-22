/**
 * @file server/src/collectors/metrics_collector.cpp
 *
 * @author Roman Snitko
 * @date 2026-04-17
 *
 * @brief MetricsCollector implementation.
 */

#include "server/collectors/metrics_collector.hpp"

#include <chrono>

namespace web_htop::server::collectors {

namespace {

[[nodiscard]] std::uint64_t CurrentTimestampMs() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

}  // namespace

web_htop::models::SystemSnapshot MetricsCollector::Collect() {
    web_htop::models::SystemSnapshot snapshot{};
    snapshot.timestamp = CurrentTimestampMs();
    snapshot.cpu = cpu_collector_.Collect();
    snapshot.memory = memory_collector_.Collect();
    snapshot.disk = disk_collector_.Collect();
    snapshot.network = network_collector_.Collect();
    snapshot.loadavg = loadavg_collector_.Collect();
    snapshot.process = process_collector_.Collect();
    return snapshot;
}

}  // namespace web_htop::server::collectors
