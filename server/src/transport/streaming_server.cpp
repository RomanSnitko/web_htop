/**
 * @file server/src/transport/streaming_server.cpp
 * 
 * @author Roman Snitko
 * @date 2026-04-16
 * 
 * @brief Implementation unit for `server/transport/streaming_server.hpp`.
 * @details Handles streaming client accept and snapshot broadcast loops.
 */

#include "server/transport/streaming_server.hpp"

#include "server/state/shared_state.hpp"
#include "server/transport/client_session.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace web_htop::server {

StreamingServer::StreamingServer(int port, SharedState* state) : port_(port), state_(state) {}

StreamingServer::~StreamingServer() {
    stop();
}

void StreamingServer::start() {
    if (running_.load()) {
        return;
    }

    stopping_.store(false);
    initialized_ = false;

    accept_thread_ = std::thread([this]() { accept_loop(); });
    {
        std::unique_lock<std::mutex> lock(ready_mutex_);
        (void)ready_cv_.wait_for(lock, std::chrono::seconds(2), [this] { return initialized_; });
    }
    if (!initialized_) {
        stopping_.store(true);
        if (accept_thread_.joinable()) {
            accept_thread_.join();
        }
        return;
    }

    running_.store(true);
    publish_thread_ = std::thread([this]() { publish_loop(); });
}

void StreamingServer::stop() {
    if (!running_.load() && !accept_thread_.joinable() && !publish_thread_.joinable()) {
        return;
    }

    stopping_.store(true);

    if (listen_fd_ >= 0) {
        close(listen_fd_);
        listen_fd_ = -1;
    }

    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
    if (publish_thread_.joinable()) {
        publish_thread_.join();
    }

    broadcaster_.StopAll();
    running_.store(false);
}

bool StreamingServer::is_running() const noexcept {
    return running_.load();
}

bool StreamingServer::setup_socket() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        std::cerr << "StreamingServer: failed to create socket: " << std::strerror(errno)
                  << std::endl;
        return false;
    }

    int opt = 1;
    if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "StreamingServer: failed to set socket options: " << std::strerror(errno)
                  << std::endl;
        close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }

    sockaddr_in server_addr {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "StreamingServer: failed to bind port " << port_ << ": "
                  << std::strerror(errno) << std::endl;
        close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }

    if (listen(listen_fd_, 16) < 0) {
        std::cerr << "StreamingServer: failed to listen: " << std::strerror(errno) << std::endl;
        close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }

    const int flags = fcntl(listen_fd_, F_GETFL, 0);
    if (flags >= 0) {
        (void)fcntl(listen_fd_, F_SETFL, flags | O_NONBLOCK);
    }

    return true;
}

void StreamingServer::accept_loop() {
    if (!setup_socket()) {
        std::unique_lock<std::mutex> lock(ready_mutex_);
        initialized_ = false;
        ready_cv_.notify_all();
        return;
    }

    {
        std::unique_lock<std::mutex> lock(ready_mutex_);
        initialized_ = true;
        ready_cv_.notify_all();
    }

    while (!stopping_.load()) {
        sockaddr_in client_addr {};
        socklen_t addr_len = sizeof(client_addr);

        const int client_fd =
            accept(listen_fd_, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            if (!stopping_.load()) {
                std::cerr << "StreamingServer: accept failed: " << std::strerror(errno)
                          << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        broadcaster_.AddSession(std::make_shared<ClientSession>(client_fd));
    }
}

void StreamingServer::publish_loop() {
    while (!stopping_.load()) {
        if (state_ != nullptr) {
            const auto snapshot = state_->get_system_snapshot();
            const std::string payload = snapshot.ToJson().ToString();
            broadcaster_.Broadcast(payload);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

}
