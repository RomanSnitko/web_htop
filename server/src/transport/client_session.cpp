/**
 * @file server/src/transport/client_session.cpp
 * 
 * @author Roman Snitko
 * @date 2026-04-16
 * 
 * @brief Implementation unit for `server/transport/client_session.hpp`.
 * @details Owns a client socket and sends framed payloads.
 */

#include "server/transport/client_session.hpp"
#include "server/transport/tcp_message_framer.hpp"

#include <unistd.h>

namespace web_htop::server {

ClientSession::ClientSession(int fd) : fd_(fd) {}

ClientSession::~ClientSession() {
    Close();
}

bool ClientSession::Send(std::string_view payload) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    if (!alive_.load() || fd_ < 0) {
        return false;
    }
    if (!TcpMessageFramer::WriteFrame(fd_, payload)) {
        alive_.store(false);
        return false;
    }
    return true;
}

void ClientSession::Close() {
    const bool was_alive = alive_.exchange(false);
    if (was_alive && fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

bool ClientSession::IsAlive() const noexcept {
    return alive_.load();
}

}
