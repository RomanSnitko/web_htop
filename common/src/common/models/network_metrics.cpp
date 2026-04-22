/**
 * @file common/models/network_metrics.cpp
 *
 * @author Maksim Vashkevich
 * @date 2026-04-18
 *
 * @brief Network metrics model serialization implementation.
 * @details Provides conversion between `NetworkMetrics` and internal JSON
 * model.
 */

#include "common/models/network_metrics.hpp"

namespace web_htop::models {

json::utils::JSONValue NetworkMetrics::ToJson() const {
    json::utils::JSONObject object{};
    object.reserve(5);

    object.emplace_back("timestamp", json::utils::JSONValue(static_cast<std::uint64_t>(timestamp)));
    object.emplace_back("rx_bytes_total", json::utils::JSONValue(rx_bytes_total));
    object.emplace_back("tx_bytes_total", json::utils::JSONValue(tx_bytes_total));
    object.emplace_back("rx_kbps", json::utils::JSONValue(rx_kbps));
    object.emplace_back("tx_kbps", json::utils::JSONValue(tx_kbps));

    return json::utils::JSONValue(std::move(object));
}

NetworkMetrics NetworkMetrics::FromJson(json::utils::JSONValue const & value) {
    NetworkMetrics metrics{};

    if (auto ts = value["timestamp"]) {
        if (auto parsed = ts->get().AsUInt64()) {
            metrics.timestamp = *parsed;
        }
    }
    if (auto rx_total = value["rx_bytes_total"]) {
        if (auto parsed = rx_total->get().AsUInt64()) {
            metrics.rx_bytes_total = *parsed;
        }
    }
    if (auto tx_total = value["tx_bytes_total"]) {
        if (auto parsed = tx_total->get().AsUInt64()) {
            metrics.tx_bytes_total = *parsed;
        }
    }
    if (auto rx_rate = value["rx_kbps"]) {
        if (auto parsed = rx_rate->get().AsDouble()) {
            metrics.rx_kbps = *parsed;
        }
    }
    if (auto tx_rate = value["tx_kbps"]) {
        if (auto parsed = tx_rate->get().AsDouble()) {
            metrics.tx_kbps = *parsed;
        }
    }

    return metrics;
}

} // namespace web_htop::models
