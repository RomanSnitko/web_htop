/**
 * @file server/include/server/config/server_config.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-16
 *
 * @brief Base runtime config for server-core.
 */

#pragma once

#include <string>

namespace web_htop::server {

struct ServerConfig {
    int port = 8080;
    int streaming_port = 9999;
    int poll_interval_ms = 1000;  ///< Metrics update interval, milliseconds.
    int max_processes = 1024;     ///< Max process entries in responses.
    std::string log_level = "info";
};

}
