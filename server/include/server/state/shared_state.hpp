/**
 * @file server/include/server/state/shared_state.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-16
 *
 * @brief Shared runtime state used by server modules.
 */

#pragma once

#include "common/models/system_snapshot.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace web_htop::server {

/**
 * @brief Placeholder process row until process module defines real type.
 */
struct ProcessEntryStub {
    int pid = 0;
    std::string name;
};

class SharedState {
public:
    SharedState() = default;

    void reset();

    void set_running(bool value) noexcept;
    bool is_running() const noexcept;

    /** @brief Writers (collectors) update; readers (HTTP/streaming) copy out. */
    void update_system_snapshot(const web_htop::models::SystemSnapshot& snapshot);

    [[nodiscard]] web_htop::models::SystemSnapshot get_system_snapshot() const;

    void update_processes(const std::vector<ProcessEntryStub>& processes);
    [[nodiscard]] std::vector<ProcessEntryStub> get_processes() const;

    void set_module_ready(const std::string& module_name, bool ready);
    [[nodiscard]] bool is_module_ready(const std::string& module_name) const;

private:
    std::atomic<bool> running_{false};
    mutable std::mutex mutex_;

    web_htop::models::SystemSnapshot snapshot_{};
    std::vector<ProcessEntryStub> processes_{};
    std::unordered_map<std::string, bool> module_ready_{};
};

}  // namespace web_htop::server
