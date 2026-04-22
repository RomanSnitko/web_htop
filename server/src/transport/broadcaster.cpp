/**
 * @file server/src/transport/broadcaster.cpp
 * 
 * @author Roman Snitko
 * @date 2026-04-16
 * 
 * @brief Implementation unit for `server/transport/broadcaster.hpp`.
 * @details Keeps active streaming sessions and broadcasts payloads.
 */

#include "server/transport/broadcaster.hpp"
#include "server/transport/client_session.hpp"

#include <algorithm>

namespace web_htop::server {

void Broadcaster::AddSession(const std::shared_ptr<ClientSession>& session) {
    if (!session) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.push_back(session);
}

void Broadcaster::Broadcast(std::string_view payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& session : sessions_) {
        if (!session->IsAlive()) {
            continue;
        }
        (void)session->Send(payload);
    }

    sessions_.erase(std::remove_if(sessions_.begin(), sessions_.end(),
                                   [](const std::shared_ptr<ClientSession>& session) {
                                       return !session || !session->IsAlive();
                                   }),
                    sessions_.end());
}

void Broadcaster::StopAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& session : sessions_) {
        if (session) {
            session->Close();
        }
    }
    sessions_.clear();
}

std::size_t Broadcaster::SessionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

}
