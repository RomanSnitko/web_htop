/**
 * @file server/include/server/app/server_app.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-16
 *
 * @brief Server app orchestrator declaration.
 * @details Coordinates ConfigManager, SharedState and server subsystems.
 */

#pragma once

#include "server/config/config_manager.hpp"
#include "server/state/shared_state.hpp"
#include "server/collectors/metrics_collector.hpp"
#include <atomic>
#include <memory>

namespace web_htop::server {

class HttpServer;
class StreamingServer;

/**
 * @brief Main lifecycle coordinator for server-core.
 */
class ServerApp {
public:
    /**
     * @brief Creates app with shared dependencies.
     * @param cfg_mgr Access to runtime config.
     * @param state Access to shared runtime state.
     */
    explicit ServerApp(ConfigManager* cfg_mgr, SharedState* state);
    ~ServerApp();

    /**
     * @brief Runs init -> start subsystems -> wait -> stop subsystems.
     * @returns Exit code for main().
     */
    int run();

    /**
     * @brief Requests graceful stop from another context (signal/thread).
     */
    void stop();

private:
    /** @brief Prepares initial state before starting subsystems. */
    void init();

    /** @brief Starts managed subsystems (HTTP now, others later). */
    void StartSubsystems();

    /** @brief Stops managed subsystems and releases resources. */
    void StopSubsystems();

    /** @brief Validates config values used at startup. */
    void ValidationConfig() const;

    ConfigManager* cfg_mgr = nullptr;            ///< Non-owning config manager.
    SharedState* state = nullptr;                ///< Non-owning shared state.
    std::atomic<bool> stopping{false};           ///< Shutdown request flag.
    std::unique_ptr<HttpServer> http_server;     ///< Managed HTTP subsystem.
    std::unique_ptr<StreamingServer> streaming_server;  ///< Managed streaming subsystem.
    collectors::MetricsCollector metrics_collector_{};  ///< Fills SharedState snapshot.
};

}
