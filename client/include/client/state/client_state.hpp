#pragma once

#include "common/models/system_snapshot.hpp"

#include <mutex>
#include <string>

namespace web_htop::client {

class ClientState {
public:
    void UpdateSnapshot(const web_htop::models::SystemSnapshot& snapshot);
    [[nodiscard]] web_htop::models::SystemSnapshot GetSnapshot() const;

    void SetConnected(bool connected);
    [[nodiscard]] bool IsConnected() const;

    void SetConnectionMessage(std::string message);
    [[nodiscard]] std::string GetConnectionMessage() const;

private:
    mutable std::mutex mutex_{};
    web_htop::models::SystemSnapshot snapshot_{};
    bool connected_{false};
    std::string connection_message_{"starting"};
};

}  // namespace web_htop::client
