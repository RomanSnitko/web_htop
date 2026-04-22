#pragma once

#include <string>

namespace web_htop::client {

class ServerConnection {
public:
    ServerConnection(std::string host, int port);
    ~ServerConnection();

    [[nodiscard]] bool Connect();
    void Disconnect();
    [[nodiscard]] bool IsConnected() const noexcept;

    [[nodiscard]] int socket_fd() const noexcept;
    [[nodiscard]] const std::string& host() const noexcept;
    [[nodiscard]] int port() const noexcept;

private:
    std::string host_{};
    int port_{0};
    int socket_fd_{-1};
};

}  // namespace web_htop::client
