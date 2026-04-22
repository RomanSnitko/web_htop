#include "client/app/client_app.hpp"

#include <chrono>
#include <iostream>
#include <thread>

namespace web_htop::client {

ClientApp::ClientApp(std::string host, int streaming_port, int http_port)
    : context_(std::move(host), streaming_port, http_port) {}

bool ClientApp::connect_stream(unsigned int attempt) {
    if (context_.connection.Connect()) {
        context_.state.SetConnected(true);
        context_.state.SetConnectionMessage("connected");
        return true;
    }

    context_.state.SetConnected(false);
    context_.state.SetConnectionMessage("reconnecting");

    web_htop::models::SystemSnapshot snapshot{};
    if (context_.http_control.FetchMetrics(snapshot)) {
        context_.state.UpdateSnapshot(snapshot);
    }

    std::this_thread::sleep_for(reconnect_policy_.NextDelay(attempt));
    return false;
}

void ClientApp::render_once() {
    context_.terminal_ui.Render(context_.state);
}

int ClientApp::run() {
    unsigned int attempt = 0;

    while (!stopping_.load()) {
        if (!context_.connection.IsConnected()) {
            if (!connect_stream(attempt)) {
                ++attempt;
                render_once();
                continue;
            }
            attempt = 0;
            render_once();
        }

        web_htop::models::SystemSnapshot snapshot{};
        if (!context_.stream_reader.ReadSnapshot(snapshot)) {
            context_.connection.Disconnect();
            context_.state.SetConnected(false);
            context_.state.SetConnectionMessage("disconnected");
            render_once();
            continue;
        }

        context_.state.UpdateSnapshot(snapshot);
        render_once();
    }

    context_.connection.Disconnect();
    context_.state.SetConnected(false);
    context_.state.SetConnectionMessage("stopped");
    render_once();
    return 0;
}

void ClientApp::stop() {
    stopping_.store(true);
}

}  // namespace web_htop::client
