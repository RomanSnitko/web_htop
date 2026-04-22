/**
 * @file server/src/main.cpp
 *
 * @author Roman Snitko
 * @date 2026-04-16
 *
 * @brief Entry point for server process.
 * @details Creates config/state, starts ServerApp, handles fatal errors.
 */

#include <iostream>
#include <memory>
#include <stdexcept>
#include <csignal>
#include <cstdlib>

#include "server/config/server_config.hpp"
#include "server/config/config_manager.hpp"
#include "server/app/server_app.hpp"
#include "server/state/shared_state.hpp"

using web_htop::server::ServerApp;
using web_htop::server::ServerConfig;
using web_htop::server::ConfigManager;
using web_htop::server::SharedState;

/** @brief Used to call stop() from signal handler. */
static ServerApp* g_server_instance = nullptr;

/** @brief Handles SIGINT/SIGTERM and requests graceful stop. */
void SignalHandler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        std::cout << "\nSignal received, requesting shutdown..." << std::endl;
        if (g_server_instance) { 
            g_server_instance->stop();
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        (void)argc;
        (void)argv;
        std::cout << "Web Htop Server starting..." << std::endl;

        // register shutdown handlers
        std::signal(SIGINT, SignalHandler);
        std::signal(SIGTERM, SignalHandler);
        std::signal(SIGPIPE, SIG_IGN);

        // create base config + manager his (for the future extension.. maybe)
        ServerConfig cfg;
        if (const char* http_port = std::getenv("WEB_HTOP_HTTP_PORT")) {
            cfg.port = std::atoi(http_port);
        }
        if (const char* streaming_port = std::getenv("WEB_HTOP_STREAMING_PORT")) {
            cfg.streaming_port = std::atoi(streaming_port);
        }
        ConfigManager cfg_mgr(cfg);

        // shared runtime state
        SharedState state;

        // app orchestrator
        ServerApp server(&cfg_mgr, &state);
        g_server_instance = &server;

        int rc = server.run();
        std::cout << "Server exited with code " << rc << std::endl;
        return rc;

    } catch (const std::exception& e) {
        std::cerr << "Critical error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Critical error: unknown exception" << std::endl;
        return 1;
    }
}
