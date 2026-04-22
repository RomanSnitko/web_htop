/**
 * @file common/models/memory_metrics.cpp
 *
 * @author Maksim Vashkevich
 * @date 2026-04-08
 *
 * @brief Memory metrics model serialization implementation.
 * @details Provides conversion between `MemoryMetrics` and internal JSON model.
 */

#include "common/models/memory_metrics.hpp"

namespace web_htop::models {
 
json::utils::JSONValue MemoryMetrics::ToJson() const {
    json::utils::JSONObject object{};
    object.reserve(5);
 
    object.emplace_back("timestamp", json::utils::JSONValue(static_cast<std::uint64_t>(timestamp)));
    object.emplace_back("total_bytes", json::utils::JSONValue(total_bytes));
    object.emplace_back("available_bytes", json::utils::JSONValue(available_bytes));
    object.emplace_back("used_bytes", json::utils::JSONValue(used_bytes));
    object.emplace_back("used_percent", json::utils::JSONValue(used_percent));
     
    return json::utils::JSONValue(std::move(object));
}
 
MemoryMetrics MemoryMetrics::FromJson(json::utils::JSONValue const & value) {
    MemoryMetrics metrics{};
 
    if (auto ts = value["timestamp"]) {
        if (auto parsed = ts->get().AsUInt64()) {
            metrics.timestamp = *parsed;
        }
    }
    if (auto total = value["total_bytes"]) {
        if (auto parsed = total->get().AsUInt64()) {
            metrics.total_bytes = *parsed;
        }
    }
    if (auto available = value["available_bytes"]) {
        if (auto parsed = available->get().AsUInt64()) {
            metrics.available_bytes = *parsed;
        }
    }
    if (auto used = value["used_bytes"]) {
        if (auto parsed = used->get().AsUInt64()) {
            metrics.used_bytes = *parsed;
        }
    }
    if (auto usage = value["used_percent"]) {
        if (auto parsed = usage->get().AsDouble()) {
            metrics.used_percent = *parsed;
        }
    }
 
    return metrics;
}
 
} // namespace web_htop::models 