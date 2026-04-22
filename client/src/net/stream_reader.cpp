#include "client/net/stream_reader.hpp"

#include "client/net/server_connection.hpp"
#include "common/json/parser.hpp"

#include <arpa/inet.h>
#include <cstdint>
#include <sys/socket.h>

namespace web_htop::client {

StreamReader::StreamReader(ServerConnection* connection) : connection_(connection) {}

bool StreamReader::read_exact(int fd, char* data, std::size_t size) const {
    std::size_t received = 0;
    while (received < size) {
        const ssize_t n = recv(fd, data + received, size - received, 0);
        if (n <= 0) {
            return false;
        }
        received += static_cast<std::size_t>(n);
    }
    return true;
}

bool StreamReader::ReadSnapshot(web_htop::models::SystemSnapshot& snapshot) const {
    if (connection_ == nullptr || !connection_->IsConnected()) {
        return false;
    }

    std::uint32_t net_len = 0;
    if (!read_exact(connection_->socket_fd(), reinterpret_cast<char*>(&net_len), sizeof(net_len))) {
        return false;
    }
    const std::uint32_t payload_len = ntohl(net_len);
    if (payload_len == 0 || payload_len > 2 * 1024 * 1024) {
        return false;
    }

    std::string payload(payload_len, '\0');
    if (!read_exact(connection_->socket_fd(), payload.data(), payload.size())) {
        return false;
    }

    auto parsed = web_htop::json::Parse(payload);
    if (!parsed) {
        return false;
    }

    snapshot = web_htop::models::SystemSnapshot::FromJson(parsed->value);
    return true;
}

}  // namespace web_htop::client
