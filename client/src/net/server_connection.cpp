#include "client/net/server_connection.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

namespace web_htop::client {

ServerConnection::ServerConnection(std::string host, int port)
    : host_(std::move(host)), port_(port) {}

ServerConnection::~ServerConnection() {
    Disconnect();
}

bool ServerConnection::Connect() {
    Disconnect();

    struct addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* result = nullptr;
    const std::string port_text = std::to_string(port_);
    if (getaddrinfo(host_.c_str(), port_text.c_str(), &hints, &result) != 0) {
        return false;
    }

    for (struct addrinfo* addr = result; addr != nullptr; addr = addr->ai_next) {
        const int fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (fd < 0) {
            continue;
        }
        if (connect(fd, addr->ai_addr, addr->ai_addrlen) == 0) {
            socket_fd_ = fd;
            freeaddrinfo(result);
            return true;
        }
        close(fd);
    }

    freeaddrinfo(result);
    return false;
}

void ServerConnection::Disconnect() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool ServerConnection::IsConnected() const noexcept {
    return socket_fd_ >= 0;
}

int ServerConnection::socket_fd() const noexcept {
    return socket_fd_;
}

const std::string& ServerConnection::host() const noexcept {
    return host_;
}

int ServerConnection::port() const noexcept {
    return port_;
}

}  // namespace web_htop::client
