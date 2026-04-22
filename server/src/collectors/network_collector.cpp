/**
 * @file server/src/collectors/network_collector.cpp
 *
 * @author Roman Snitko
 * @date 2026-04-17
 *
 * @brief NetworkCollector implementation.
 */

#include "server/collectors/network_collector.hpp"

#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace web_htop::server::collectors {

namespace {

constexpr double kBytesPerKib = 1024.0;

}  // namespace

web_htop::models::NetworkMetrics NetworkCollector::Collect() {
    const Totals now = ReadProcNetDev();
    const std::uint64_t now_ms = CurrentTimestamp();

    web_htop::models::NetworkMetrics metrics{};
    metrics.timestamp = now_ms;
    metrics.rx_bytes_total = now.rx_bytes;
    metrics.tx_bytes_total = now.tx_bytes;

    if (previous_totals_ && previous_time_ms_) {
        const std::uint64_t dt_ms = (now_ms > *previous_time_ms_) ? (now_ms - *previous_time_ms_) : 0;
        if (dt_ms > 0) {
            const double dt_s = static_cast<double>(dt_ms) / 1000.0;
            const double drx = static_cast<double>(now.rx_bytes - previous_totals_->rx_bytes);
            const double dtx = static_cast<double>(now.tx_bytes - previous_totals_->tx_bytes);
            metrics.rx_kbps = (drx / dt_s) / kBytesPerKib;
            metrics.tx_kbps = (dtx / dt_s) / kBytesPerKib;
        }
    }

    previous_totals_ = now;
    previous_time_ms_ = now_ms;
    return metrics;
}

NetworkCollector::Totals NetworkCollector::ReadProcNetDev() const {
    std::ifstream file("/proc/net/dev");
    std::string line;
    Totals sum{};

    // Skip header lines
    if (!std::getline(file, line)) {
        return sum;
    }
    if (!std::getline(file, line)) {
        return sum;
    }

    while (std::getline(file, line)) {
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string iface = line.substr(0, colon);
        // trim spaces
        const auto not_space_front = iface.find_first_not_of(" \t");
        if (not_space_front == std::string::npos) {
            continue;
        }
        iface = iface.substr(not_space_front);
        if (iface == "lo") {
            continue;
        }

        std::istringstream iss(line.substr(colon + 1));
        std::vector<std::uint64_t> fields;
        fields.reserve(16);
        std::uint64_t v = 0;
        while (iss >> v) {
            fields.push_back(v);
        }
        if (fields.size() < 9) {
            continue;
        }
        sum.rx_bytes += fields[0];
        sum.tx_bytes += fields[8];
    }

    return sum;
}

std::uint64_t NetworkCollector::CurrentTimestamp() const {
    const auto now = std::chrono::system_clock::now();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

}  // namespace web_htop::server::collectors
