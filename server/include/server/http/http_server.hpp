/**
 * @file server/include/server/http/http_server.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-16
 *
 * @brief HTTP server interface.
 * @details Used by ServerApp to start/stop HTTP subsystem thread.
 */

#pragma once

#include "server/http/http_router.hpp"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <string>
#include <mutex>
#include <thread>

namespace web_htop::server {

class SharedState;
class HttpHandlers;
class DtoMapper;

/**
 * @brief Minimal async HTTP server wrapper.
 */
class HttpServer {
public:
    /**
     * @brief Construct server bound to a TCP port.
     * @param port Listening port.
     */
    HttpServer(int port, SharedState* state);
    ~HttpServer();

    /** @brief Starts worker thread. */
    void start();
    /** @brief Stops worker thread and joins it. */
    void stop();
    /** @brief Returns running state. */
    bool is_running() const noexcept;
    /** @brief Returns configured port. */
    int GetPort() const;

private:
    /** @brief Internal loop for request handling. */
    void run_loop();
    [[nodiscard]] bool read_request(int client_fd, std::string& raw_request) const;
    [[nodiscard]] bool parse_request_line(const std::string& raw_request, HttpRequest& request) const;
    [[nodiscard]] std::string build_response(const HttpResponse& response) const;

    int port_ = 8080;                           ///< Configured listening port.
    int listen_fd_ = -1;                        ///< OS socket descriptor.
    std::atomic<bool> running_{false};          ///< Worker active state.
    std::atomic<bool> stopping_{false};         ///< Shutdown request flag.
    std::unique_ptr<std::thread> thread_;       ///< Worker loop thread.
    mutable std::mutex ready_mutex_;            ///< Protects ready_cv_ and initialized_.
    std::condition_variable ready_cv_;          ///< Signaled when server is initialized.
    bool initialized_ = false;                  ///< True when socket is listening.
    SharedState* state_ = nullptr;              ///< Non-owning shared state.
    std::unique_ptr<DtoMapper> dto_mapper_;     ///< JSON DTO conversion.
    std::unique_ptr<HttpHandlers> handlers_;    ///< HTTP route handlers.
    std::unique_ptr<HttpRouter> router_;        ///< Route dispatcher.
};

}
