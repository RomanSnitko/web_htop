/**
 * @file server/src/state/shared_state.cpp
 *
 * @author Roman Snitko
 * @date 2026-04-16
 *
 * @brief SharedState implementation.
 */

#include "server/state/shared_state.hpp"

namespace web_htop::server {

void SharedState::reset() {
    running_.store(false);
    std::lock_guard<std::mutex> lk(mutex_);
    snapshot_ = {};
    processes_.clear();
    module_ready_.clear();
}

void SharedState::set_running(bool value) noexcept {
    running_.store(value);
}

bool SharedState::is_running() const noexcept {
    return running_.load();
}

void SharedState::update_system_snapshot(const web_htop::models::SystemSnapshot& snapshot) {
    std::lock_guard<std::mutex> lk(mutex_);
    snapshot_ = snapshot;
}

web_htop::models::SystemSnapshot SharedState::get_system_snapshot() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return snapshot_;
}

void SharedState::update_processes(const std::vector<ProcessEntryStub>& processes) {
    std::lock_guard<std::mutex> lk(mutex_);
    processes_ = processes;
}

std::vector<ProcessEntryStub> SharedState::get_processes() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return processes_;
}

void SharedState::set_module_ready(const std::string& module_name, bool ready) {
    std::lock_guard<std::mutex> lk(mutex_);
    module_ready_[module_name] = ready;
}

bool SharedState::is_module_ready(const std::string& module_name) const {
    std::lock_guard<std::mutex> lk(mutex_);
    const auto it = module_ready_.find(module_name);
    if (it == module_ready_.end()) {
        return false;
    }
    return it->second;
}

}  // namespace web_htop::server
