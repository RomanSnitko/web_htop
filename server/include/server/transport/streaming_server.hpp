/**
 * @file server/include/server/transport/streaming_server.hpp
 * 
 * @author Roman Snitko
 * @date 2026-04-16
 * 
 * @brief Streaming server transport interface.
 * @details Accepts clients and broadcasts live snapshots from SharedState.
 */

#pragma once

#include "server/transport/broadcaster.hpp"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace web_htop::server {

class SharedState;

class StreamingServer {
public:
    StreamingServer(int port, SharedState* state);
    ~StreamingServer();

    void start();
    void stop();
    [[nodiscard]] bool is_running() const noexcept;

private:
    void accept_loop();
    void publish_loop();
    [[nodiscard]] bool setup_socket();

    int port_{9999};
    int listen_fd_{-1};
    SharedState* state_{nullptr};  ///< Non-owning shared runtime state.
    Broadcaster broadcaster_{};

    std::atomic<bool> running_{false};
    std::atomic<bool> stopping_{false};
    std::thread accept_thread_{};
    std::thread publish_thread_{};

    mutable std::mutex ready_mutex_;
    std::condition_variable ready_cv_;
    bool initialized_{false};
};

}
