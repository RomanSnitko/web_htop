/**
 * @file common/models/process_metrics.cpp
 *
 * @author Maksim Vashkevich
 * @date 2026-04-18
 *
 * @brief Process list metrics model serialization implementation.
 * @details Provides conversion between `ProcessInfo`/`ProcessMetrics` and
 * internal JSON model.
 */

#include "common/models/process_metrics.hpp"

#include <string_view> // std::string_view

namespace web_htop::models {

json::utils::JSONValue ProcessInfo::ToJson() const {
    json::utils::JSONObject object{};
    object.reserve(7);

    object.emplace_back("pid", json::utils::JSONValue(static_cast<std::int64_t>(pid)));
    object.emplace_back("name", json::utils::JSONValue(std::string_view(name)));
    object.emplace_back("state", json::utils::JSONValue(static_cast<std::int64_t>(static_cast<char>(state))));
    object.emplace_back("cpu_percent", json::utils::JSONValue(cpu_percent));
    object.emplace_back("memory_bytes", json::utils::JSONValue(memory_bytes));
    object.emplace_back("memory_percent", json::utils::JSONValue(memory_percent));
    object.emplace_back("thread_count", json::utils::JSONValue(static_cast<std::uint64_t>(thread_count)));

    return json::utils::JSONValue(std::move(object));
}

ProcessInfo ProcessInfo::FromJson(json::utils::JSONValue const & value) {
    ProcessInfo info{};

    if (auto pid = value["pid"]) {
        if (auto parsed = pid->get().AsInt64()) {
            info.pid = static_cast<ProcessID>(*parsed);
        }
    }
    if (auto name = value["name"]) {
        if (auto parsed = name->get().AsString()) {
            info.name.assign(parsed->data(), parsed->size());
        }
    }
    if (auto state = value["state"]) {
        if (auto parsed = state->get().AsInt64()) {
            info.state = static_cast<ProcessState>(static_cast<char>(*parsed));
        }
    }
    if (auto cpu = value["cpu_percent"]) {
        if (auto parsed = cpu->get().AsDouble()) {
            info.cpu_percent = *parsed;
        }
    }
    if (auto memory_bytes = value["memory_bytes"]) {
        if (auto parsed = memory_bytes->get().AsUInt64()) {
            info.memory_bytes = *parsed;
        }
    }
    if (auto memory_percent = value["memory_percent"]) {
        if (auto parsed = memory_percent->get().AsDouble()) {
            info.memory_percent = *parsed;
        }
    }
    if (auto thread_count = value["thread_count"]) {
        if (auto parsed = thread_count->get().AsUInt64()) {
            info.thread_count = *parsed;
        }
    }

    return info;
}

json::utils::JSONValue ProcessMetrics::ToJson() const {
    json::utils::JSONArray processes_array{};
    processes_array.reserve(processes.size());
    for (auto const & process : processes) {
        processes_array.emplace_back(process.ToJson());
    }

    json::utils::JSONObject object{};
    object.reserve(4);

    object.emplace_back("timestamp", json::utils::JSONValue(static_cast<std::uint64_t>(timestamp)));
    object.emplace_back("total_processes", json::utils::JSONValue(total_processes));
    object.emplace_back("running_processes", json::utils::JSONValue(running_processes));
    object.emplace_back("processes", json::utils::JSONValue(std::move(processes_array)));

    return json::utils::JSONValue(std::move(object));
}

ProcessMetrics ProcessMetrics::FromJson(json::utils::JSONValue const & value) {
    ProcessMetrics metrics{};

    if (auto ts = value["timestamp"]) {
        if (auto parsed = ts->get().AsUInt64()) {
            metrics.timestamp = *parsed;
        }
    }
    if (auto total = value["total_processes"]) {
        if (auto parsed = total->get().AsUInt64()) {
            metrics.total_processes = *parsed;
        }
    }
    if (auto running = value["running_processes"]) {
        if (auto parsed = running->get().AsUInt64()) {
            metrics.running_processes = *parsed;
        }
    }
    if (auto processes = value["processes"]) {
        if (auto const * array = processes->get().AsArray()) {
            metrics.processes.reserve(array->size());
            for (auto const & process_value : *array) {
                metrics.processes.push_back(ProcessInfo::FromJson(process_value));
            }
        }
    }

    return metrics;
}

} // namespace web_htop::models
