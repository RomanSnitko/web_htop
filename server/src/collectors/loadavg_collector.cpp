/**
 * @file server/src/collectors/loadavg_collector.cpp
 *
 * @author Roman Snitko
 * @date 2026-04-17
 *
 * @brief LoadavgCollector implementation.
 */

#include "server/collectors/loadavg_collector.hpp"

#include <chrono>
#include <fstream>
#include <sstream>
#include <string>

namespace web_htop::server::collectors {

web_htop::models::LoadavgMetrics LoadavgCollector::Collect() {
    web_htop::models::LoadavgMetrics metrics{};
    metrics.timestamp = CurrentTimestamp();

    std::ifstream file("/proc/loadavg");
    std::string line;
    if (!std::getline(file, line)) {
        return metrics;
    }

    std::istringstream iss(line);
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
    if (iss >> a >> b >> c) {
        metrics.load_1m = a;
        metrics.load_5m = b;
        metrics.load_15m = c;
    }
    return metrics;
}

std::uint64_t LoadavgCollector::CurrentTimestamp() const {
    const auto now = std::chrono::system_clock::now();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

}  // namespace web_htop::server::collectors
