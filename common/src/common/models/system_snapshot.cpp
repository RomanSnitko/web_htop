/**
 * @file common/models/system_snapshot.cpp
 *
 * @author Maxim Vashkevich
 * @date 2026-04-20
 *
 * @brief System snapshot model serialization implementation.
 * @details Provides conversion between `SystemSnapshot` and internal JSON
 * model.
 */

#include "common/models/system_snapshot.hpp"

namespace web_htop::models {

json::utils::JSONValue SystemSnapshot::ToJson() const {
    json::utils::JSONObject object{};
    object.reserve(7);

    object.emplace_back("timestamp", json::utils::JSONValue(static_cast<std::uint64_t>(timestamp)));
    object.emplace_back("cpu", cpu.ToJson());
    object.emplace_back("memory", memory.ToJson());
    object.emplace_back("disk", disk.ToJson());
    object.emplace_back("network", network.ToJson());
    object.emplace_back("loadavg", loadavg.ToJson());
    object.emplace_back("process", process.ToJson());

    return json::utils::JSONValue(std::move(object));
}

SystemSnapshot SystemSnapshot::FromJson(json::utils::JSONValue const & value) {
    SystemSnapshot snapshot{};

    if (auto ts = value["timestamp"]) {
        if (auto parsed = ts->get().AsUInt64()) {
            snapshot.timestamp = *parsed;
        }
    }
    if (auto cpu = value["cpu"]) {
        snapshot.cpu = CPUMetrics::FromJson(cpu->get());
    }
    if (auto memory = value["memory"]) {
        snapshot.memory = MemoryMetrics::FromJson(memory->get());
    }
    if (auto disk = value["disk"]) {
        snapshot.disk = DiskMetrics::FromJson(disk->get());
    }
    if (auto network = value["network"]) {
        snapshot.network = NetworkMetrics::FromJson(network->get());
    }
    if (auto loadavg = value["loadavg"]) {
        snapshot.loadavg = LoadavgMetrics::FromJson(loadavg->get());
    }
    if (auto process = value["process"]) {
        snapshot.process = ProcessMetrics::FromJson(process->get());
    }

    return snapshot;
}

} // namespace web_htop::models
