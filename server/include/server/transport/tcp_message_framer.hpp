/**
 * @file server/include/server/transport/tcp_message_framer.hpp
 * 
 * @author Roman Snitko
 * @date 2026-04-16
 * 
 * @brief Tcp message framing interface.
 * @details Provides size-prefixed framing for streaming transport messages.
 */

#pragma once

#include <string>
#include <string_view>

namespace web_htop::server {

class TcpMessageFramer {
public:
    [[nodiscard]] static std::string Frame(std::string_view payload);
    [[nodiscard]] static bool WriteFrame(int fd, std::string_view payload);
};

}
