/**
 * @file common/models/loadavg_metrics.cpp
 *
 * @author Maxim Vashkevich
 * @date 2026-04-20
 *
 * @brief Load average metrics model serialization implementation.
 * @details Provides conversion between `LoadavgMetrics` and internal JSON
 * model.
 */

#include "common/models/loadavg_metrics.hpp"

namespace web_htop::models {

json::utils::JSONValue LoadavgMetrics::ToJson() const {
    json::utils::JSONObject object{};
    object.reserve(4);

    object.emplace_back("timestamp", json::utils::JSONValue(static_cast<std::uint64_t>(timestamp)));
    object.emplace_back("load_1m", json::utils::JSONValue(load_1m));
    object.emplace_back("load_5m", json::utils::JSONValue(load_5m));
    object.emplace_back("load_15m", json::utils::JSONValue(load_15m));

    return json::utils::JSONValue(std::move(object));
}

LoadavgMetrics LoadavgMetrics::FromJson(json::utils::JSONValue const & value) {
    LoadavgMetrics metrics{};

    if (auto ts = value["timestamp"]) {
        if (auto parsed = ts->get().AsUInt64()) {
            metrics.timestamp = *parsed;
        }
    }
    if (auto load_1m = value["load_1m"]) {
        if (auto parsed = load_1m->get().AsDouble()) {
            metrics.load_1m = *parsed;
        }
    }
    if (auto load_5m = value["load_5m"]) {
        if (auto parsed = load_5m->get().AsDouble()) {
            metrics.load_5m = *parsed;
        }
    }
    if (auto load_15m = value["load_15m"]) {
        if (auto parsed = load_15m->get().AsDouble()) {
            metrics.load_15m = *parsed;
        }
    }

    return metrics;
}

} // namespace web_htop::models
