#pragma once

#include "client/app/client_context.hpp"
#include "client/net/reconnect_policy.hpp"

#include <atomic>
#include <string>

namespace web_htop::client {

class ClientApp {
public:
    ClientApp(std::string host, int streaming_port, int http_port);

    int run();
    void stop();

private:
    bool connect_stream(unsigned int attempt);
    void render_once();

    std::atomic<bool> stopping_{false};
    ClientContext context_;
    ReconnectPolicy reconnect_policy_{};
};

}  // namespace web_htop::client
