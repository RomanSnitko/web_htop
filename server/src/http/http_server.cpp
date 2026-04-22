/**
 * @file server/src/http/http_server.cpp
 *
 * @author Roman Snitko
 * @date 2026-04-16
 *
 * @brief HTTP server bootstrap implementation.
 * @details Implements socket accept loop, HTTP request parsing and routing.
 */

#include "server/http/http_server.hpp"

#include "server/http/dto_mapper.hpp"
#include "server/http/http_handlers.hpp"
#include "server/state/shared_state.hpp"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace web_htop::server {

namespace {

constexpr std::size_t kMaxRequestSize = 8 * 1024;

}  // namespace

HttpServer::HttpServer(int port, SharedState* state) : port_(port), state_(state) {
    dto_mapper_ = std::make_unique<DtoMapper>();
    handlers_ = std::make_unique<HttpHandlers>(state_, dto_mapper_.get());
    router_ = std::make_unique<HttpRouter>(handlers_.get());
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    if (running_.load()) {
        return;
    }
    stopping_.store(false);
    initialized_ = false;
    thread_ = std::make_unique<std::thread>([this]() { run_loop(); });

    {
        std::unique_lock<std::mutex> lock(ready_mutex_);
        (void)ready_cv_.wait_for(lock, std::chrono::seconds(2), [this] { return initialized_; });
        running_.store(initialized_);
    }
}

void HttpServer::stop() {
    if (!running_.load() && !thread_) {
        return;
    }

    stopping_.store(true);
    if (listen_fd_ >= 0) {
        close(listen_fd_);
        listen_fd_ = -1;
    }
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    thread_.reset();
    running_.store(false);
}

bool HttpServer::is_running() const noexcept {
    return running_.load();
}

bool HttpServer::read_request(int client_fd, std::string& raw_request) const {
    raw_request.clear();
    raw_request.reserve(kMaxRequestSize);

    char buffer[1024];
    while (raw_request.find("\r\n\r\n") == std::string::npos) {
        const ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);
        if (n <= 0) {
            return false;
        }
        raw_request.append(buffer, static_cast<std::size_t>(n));
        if (raw_request.size() > kMaxRequestSize) {
            return false;
        }
    }

    return true;
}

bool HttpServer::parse_request_line(const std::string& raw_request, HttpRequest& request) const {
    const std::size_t line_end = raw_request.find("\r\n");
    if (line_end == std::string::npos) {
        return false;
    }

    std::istringstream line_stream(raw_request.substr(0, line_end));
    if (!(line_stream >> request.method >> request.path >> request.version)) {
        return false;
    }
    if (request.method.empty() || request.path.empty() || request.version.empty()) {
        return false;
    }

    return true;
}

std::string HttpServer::build_response(const HttpResponse& response) const {
    std::ostringstream out;
    out << "HTTP/1.1 " << response.status_code << ' ' << response.status_text << "\r\n";
    out << "Content-Type: " << response.content_type << "\r\n";
    out << "Content-Length: " << response.body.size() << "\r\n";
    out << "Connection: " << (response.close_connection ? "close" : "keep-alive") << "\r\n";
    out << "\r\n";
    out << response.body;
    return out.str();
}

void HttpServer::run_loop() {
    running_.store(false);

    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        std::unique_lock<std::mutex> lock(ready_mutex_);
        initialized_ = false;
        ready_cv_.notify_all();
        return;
    }

    int opt = 1;
    if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options: " << strerror(errno) << std::endl;
        close(listen_fd_);
        listen_fd_ = -1;
        std::unique_lock<std::mutex> lock(ready_mutex_);
        initialized_ = false;
        ready_cv_.notify_all();
        return;
    }

    struct sockaddr_in server_addr {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (bind(listen_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket to port " << port_ << ": " << strerror(errno) << std::endl;
        close(listen_fd_);
        listen_fd_ = -1;
        std::unique_lock<std::mutex> lock(ready_mutex_);
        initialized_ = false;
        ready_cv_.notify_all();
        return;
    }

    if (listen(listen_fd_, 5) < 0) {
        std::cerr << "Failed to listen on socket: " << strerror(errno) << std::endl;
        close(listen_fd_);
        listen_fd_ = -1;
        std::unique_lock<std::mutex> lock(ready_mutex_);
        initialized_ = false;
        ready_cv_.notify_all();
        return;
    }

    int flags = fcntl(listen_fd_, F_GETFL, 0);
    if (flags >= 0) {
        (void)fcntl(listen_fd_, F_SETFL, flags | O_NONBLOCK);
    }

    {
        std::unique_lock<std::mutex> lock(ready_mutex_);
        initialized_ = true;
        ready_cv_.notify_all();
    }
    running_.store(true);

    while (!stopping_.load()) {
        struct sockaddr_in client_addr {};
        socklen_t client_addrlen = sizeof(client_addr);

        int client_fd = accept(listen_fd_, (struct sockaddr*)&client_addr, &client_addrlen);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            if (!stopping_.load()) {
                std::cerr << "HttpServer accept failed: " << strerror(errno) << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        std::string raw_request;
        HttpResponse response;
        if (!read_request(client_fd, raw_request)) {
            response = HttpResponse{400, "Bad Request", "application/json",
                                    R"({"error":"invalid_request"})", true};
        } else {
            HttpRequest request;
            if (!parse_request_line(raw_request, request)) {
                response = HttpResponse{400, "Bad Request", "application/json",
                                        R"({"error":"invalid_request_line"})", true};
            } else {
                response = router_->Route(request);
            }
        }

        const std::string raw_response = build_response(response);
        (void)send(client_fd, raw_response.data(), raw_response.size(), 0);
        close(client_fd);
    }

    if (listen_fd_ >= 0) {
        close(listen_fd_);
        listen_fd_ = -1;
    }
    running_.store(false);
}

int HttpServer::GetPort() const {
    return port_;
}

}
