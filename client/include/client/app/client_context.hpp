#pragma once

#include "client/http/http_control_client.hpp"
#include "client/net/server_connection.hpp"
#include "client/net/stream_reader.hpp"
#include "client/state/client_state.hpp"
#include "client/ui/terminal_ui.hpp"

#include <string>

namespace web_htop::client {

struct ClientContext {
    ClientContext(std::string server_host, int streaming_port, int http_port);

    ClientState state;
    ServerConnection connection;
    StreamReader stream_reader;
    HttpControlClient http_control;
    TerminalUi terminal_ui;
};

}  // namespace web_htop::client
