/**
 * @file server/src/transport/tcp_message_framer.cpp
 * 
 * @author Roman Snitko
 * @date 2026-04-16
 * 
 * @brief Implementation unit for `server/transport/tcp_message_framer.hpp`.
 * @details Implements 4-byte big-endian length-prefixed framing.
 */

#include "server/transport/tcp_message_framer.hpp"

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

namespace web_htop::server {

std::string TcpMessageFramer::Frame(std::string_view payload) {
    const std::uint32_t len = static_cast<std::uint32_t>(payload.size());
    const std::uint32_t net_len = htonl(len);

    std::string framed;
    framed.resize(sizeof(std::uint32_t) + payload.size());
    std::memcpy(framed.data(), &net_len, sizeof(std::uint32_t));
    std::memcpy(framed.data() + sizeof(std::uint32_t), payload.data(), payload.size());
    return framed;
}

bool TcpMessageFramer::WriteFrame(int fd, std::string_view payload) {
    const std::string framed = Frame(payload);
    std::size_t sent = 0;
    while (sent < framed.size()) {
        const ssize_t n = send(fd, framed.data() + sent, framed.size() - sent, 0);
        if (n <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

}
