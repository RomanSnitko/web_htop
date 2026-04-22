/**
 * @file server/include/server/transport/client_session.hpp
 * 
 * @author Roman Snitko
 * @date 2026-04-16
 * 
 * @brief Client session transport interface.
 * @details Wraps one streaming client connection and safe send operations.
 */

#pragma once

#include <atomic>
#include <mutex>
#include <string_view>

namespace web_htop::server {

class ClientSession {
public:
    explicit ClientSession(int fd);
    ~ClientSession();

    [[nodiscard]] bool Send(std::string_view payload);
    void Close();
    [[nodiscard]] bool IsAlive() const noexcept;

private:
    int fd_{-1};
    std::atomic<bool> alive_{true};
    std::mutex write_mutex_;
};

}
