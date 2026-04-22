#include "client/app/client_app.hpp"

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

web_htop::client::ClientApp* g_client = nullptr;

void HandleSignal(int sig) {
    if ((sig == SIGINT || sig == SIGTERM) && g_client != nullptr) {
        g_client->stop();
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <streaming_port> [http_port]" << std::endl;
        return 1;
    }

    const std::string host = argv[1];
    const int streaming_port = std::atoi(argv[2]);
    const int http_port = (argc >= 4) ? std::atoi(argv[3]) : 8080;

    if (streaming_port <= 0 || http_port <= 0) {
        std::cerr << "Invalid port values." << std::endl;
        return 1;
    }

    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);
    std::signal(SIGPIPE, SIG_IGN);

    web_htop::client::ClientApp app(host, streaming_port, http_port);
    g_client = &app;
    return app.run();
}
