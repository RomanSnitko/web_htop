/**
 * @file common/models/cpu_metrics.cpp
 *
 * @author Maksim Vashkevich
 * @date 2026-04-07
 *
 * @brief CPU metrics model serialization implementation.
 * @details Provides conversion between `CPUMetrics` and internal JSON model.
 */

#include "common/models/cpu_metrics.hpp"

namespace web_htop::models {

json::utils::JSONValue CPUMetrics::ToJson() const {
    json::utils::JSONArray per_core_array{};
    per_core_array.reserve(per_core_usage_percent.size());
    for (auto const usage : per_core_usage_percent) {
        per_core_array.emplace_back(usage);
    }

    json::utils::JSONObject object{};
    object.reserve(5);
    
    object.emplace_back("timestamp", json::utils::JSONValue(static_cast<std::uint64_t>(timestamp)));
    object.emplace_back("core_count", json::utils::JSONValue(static_cast<std::uint64_t>(core_count)));
    object.emplace_back("total_usage_percent", json::utils::JSONValue(total_usage_percent));
    object.emplace_back("frequency_mhz", json::utils::JSONValue(frequency_mhz));
    object.emplace_back("per_core_usage_percent", json::utils::JSONValue(std::move(per_core_array)));

    return json::utils::JSONValue(std::move(object));
}

CPUMetrics CPUMetrics::FromJson(json::utils::JSONValue const & value) {
    CPUMetrics metrics{};

    if (auto ts = value["timestamp"]) {
        if (auto parsed = ts->get().AsUInt64()) {
            metrics.timestamp = *parsed;
        }
    }
    if (auto cores = value["core_count"]) {
        if (auto parsed = cores->get().AsUInt64()) {
            metrics.core_count = static_cast<CPUCores>(*parsed);
        }
    }
    if (auto usage = value["total_usage_percent"]) {
        if (auto parsed = usage->get().AsDouble()) {
            metrics.total_usage_percent = *parsed;
        }
    }
    if (auto freq = value["frequency_mhz"]) {
        if (auto parsed = freq->get().AsDouble()) {
            metrics.frequency_mhz = *parsed;
        }
    }
    if (auto per_core = value["per_core_usage_percent"]) {
        if (auto const * array = per_core->get().AsArray()) {
            metrics.per_core_usage_percent.reserve(array->size());
            for (auto const & item : *array) {
                if (auto item_usage = item.AsDouble()) {
                    metrics.per_core_usage_percent.push_back(*item_usage);
                }
            }
        }
    }

    return metrics;
}

} // namespace web_htop::models
