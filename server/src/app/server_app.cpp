/**
 * @file server/src/app/server_app.cpp
 *
 * @author Roman Snitko
 * @date 2026-04-16
 *
 * @brief ServerApp lifecycle implementation.
 * @details Keeps startup/shutdown flow in one place.
 */

#include "server/app/server_app.hpp"
#include "server/config/server_config.hpp"
#include "server/http/http_server.hpp"
#include "server/transport/streaming_server.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace web_htop::server {

using web_htop::server::HttpServer;

ServerApp::ServerApp(ConfigManager* cfg_mgr, SharedState* state)
    : cfg_mgr(cfg_mgr), state(state) {}

ServerApp::~ServerApp() {
    stop();
}

void ServerApp::init() {
    // reset shared state before startup
    if (state) {
        state->reset();
    }
}

void ServerApp::ValidationConfig() const {
    if (cfg_mgr == nullptr) {
        throw std::runtime_error("ConfigManager is null");
    }
    const ServerConfig cfg = cfg_mgr->get();
    if (cfg.port <= 0) {
        throw std::runtime_error("Invalid HTTP port");
    }
    if (cfg.streaming_port <= 0) {
        throw std::runtime_error("Invalid streaming port");
    }
    if (cfg.poll_interval_ms <= 0) {
        throw std::runtime_error("Invalid poll_interval_ms");
    }
    if (cfg.max_processes <= 0) {
        throw std::runtime_error("Invalid max_processes");
    }
}

void ServerApp::StartSubsystems() {
    ValidationConfig();
    ServerConfig cfg = cfg_mgr->get();
    http_server = std::make_unique<HttpServer>(cfg.port, state);
    http_server->start();
    if (!http_server->is_running()) {
        throw std::runtime_error("HTTP server failed to start");
    }

    streaming_server = std::make_unique<StreamingServer>(cfg.streaming_port, state);
    streaming_server->start();
    if (!streaming_server->is_running()) {
        throw std::runtime_error("Streaming server failed to start");
    }

    std::cout << "ServerApp: HTTP server started on port " << cfg.port << std::endl;
    std::cout << "ServerApp: Streaming server started on port " << cfg.streaming_port << std::endl;
}

void ServerApp::StopSubsystems() {
    if (streaming_server) {
        streaming_server->stop();
        streaming_server.reset();
    }
    if (http_server) {
        http_server->stop();
        http_server.reset();
    }
}

int ServerApp::run() {
    try {
        init();
        StartSubsystems();
        if (state) {
            state->set_running(true);
        }

        std::cout << "ServerApp: running" << std::endl;

        const ServerConfig cfg = cfg_mgr->get();
        const auto tick = std::chrono::milliseconds(cfg.poll_interval_ms);
        auto next_collect = std::chrono::steady_clock::now();

        while (!stopping.load()) {
            const auto now = std::chrono::steady_clock::now();
            if (state != nullptr && now >= next_collect) {
                state->update_system_snapshot(metrics_collector_.Collect());
                state->set_module_ready("collectors", true);
                next_collect += tick;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        StopSubsystems();
        if (state) {
            state->set_running(false);
        }
        std::cout << "ServerApp: stopped" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ServerApp fatal: " << e.what() << std::endl;
        return 2;
    }
}

void ServerApp::stop() {
    // can be called from signal handler path through main.cpp
    stopping.store(true);
    if (streaming_server) {
        streaming_server->stop();
    }
    if (http_server) {
        http_server->stop();
    }
}

}
