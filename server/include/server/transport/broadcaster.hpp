/**
 * @file server/include/server/transport/broadcaster.hpp
 * 
 * @author Roman Snitko
 * @date 2026-04-16
 * 
 * @brief Broadcast transport interface.
 * @details Fan-out helper sending snapshot payload to all sessions.
 */

#pragma once

#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

namespace web_htop::server {

class ClientSession;

class Broadcaster {
public:
    void AddSession(const std::shared_ptr<ClientSession>& session);
    void Broadcast(std::string_view payload);
    void StopAll();

    [[nodiscard]] std::size_t SessionCount() const;

private:
    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<ClientSession>> sessions_{};
};

}
