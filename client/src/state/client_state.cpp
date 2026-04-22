#include "client/state/client_state.hpp"

namespace web_htop::client {

void ClientState::UpdateSnapshot(const web_htop::models::SystemSnapshot& snapshot) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_ = snapshot;
}

web_htop::models::SystemSnapshot ClientState::GetSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_;
}

void ClientState::SetConnected(bool connected) {
    std::lock_guard<std::mutex> lock(mutex_);
    connected_ = connected;
}

bool ClientState::IsConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connected_;
}

void ClientState::SetConnectionMessage(std::string message) {
    std::lock_guard<std::mutex> lock(mutex_);
    connection_message_ = std::move(message);
}

std::string ClientState::GetConnectionMessage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connection_message_;
}

}  // namespace web_htop::client
