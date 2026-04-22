#include "client/http/http_control_client.hpp"

#include "common/json/parser.hpp"

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

namespace web_htop::client {

HttpControlClient::HttpControlClient(std::string host, int port)
    : host_(std::move(host)), port_(port) {}

std::optional<std::string> HttpControlClient::PerformGet(const std::string& path) const {
    struct addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* result = nullptr;
    const std::string port_text = std::to_string(port_);
    if (getaddrinfo(host_.c_str(), port_text.c_str(), &hints, &result) != 0) {
        return std::nullopt;
    }

    int fd = -1;
    for (struct addrinfo* addr = result; addr != nullptr; addr = addr->ai_next) {
        fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (fd < 0) {
            continue;
        }
        if (connect(fd, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(result);

    if (fd < 0) {
        return std::nullopt;
    }

    const std::string request = "GET " + path +
                                " HTTP/1.1\r\nHost: " + host_ +
                                "\r\nConnection: close\r\n\r\n";
    if (send(fd, request.data(), request.size(), 0) < 0) {
        close(fd);
        return std::nullopt;
    }

    std::string response;
    char buffer[2048];
    while (true) {
        const ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
        if (n <= 0) {
            break;
        }
        response.append(buffer, static_cast<std::size_t>(n));
    }
    close(fd);

    const std::size_t body_pos = response.find("\r\n\r\n");
    if (body_pos == std::string::npos) {
        return std::nullopt;
    }

    return response.substr(body_pos + 4);
}

bool HttpControlClient::FetchMetrics(web_htop::models::SystemSnapshot& snapshot) const {
    const auto body = PerformGet("/metrics");
    if (!body) {
        return false;
    }
    auto parsed = web_htop::json::Parse(*body);
    if (!parsed) {
        return false;
    }
    snapshot = web_htop::models::SystemSnapshot::FromJson(parsed->value);
    return true;
}

bool HttpControlClient::FetchProcesses(web_htop::models::ProcessMetrics& processes) const {
    const auto body = PerformGet("/processes");
    if (!body) {
        return false;
    }
    auto parsed = web_htop::json::Parse(*body);
    if (!parsed) {
        return false;
    }
    processes = web_htop::models::ProcessMetrics::FromJson(parsed->value);
    return true;
}

}  // namespace web_htop::client
