#include "client/app/client_context.hpp"

#include <utility>

namespace web_htop::client {

ClientContext::ClientContext(std::string server_host, int streaming_port, int http_port)
    : connection(std::move(server_host), streaming_port),
      stream_reader(&connection),
      http_control(connection.host(), http_port) {}

}  // namespace web_htop::client
