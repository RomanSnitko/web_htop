/**
 * @file server/include/server/config/config_manager.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-16
 *
 * @brief Thread-safe runtime config storage.
 * @details Shared between ServerApp and future HTTP/config update paths.
 */

#pragma once

#include "server_config.hpp"
#include <mutex>

namespace web_htop::server {

class ConfigManager {
public:
    ConfigManager() = default;
    explicit ConfigManager(const ServerConfig& cfgServer) : cfg(cfgServer) {}

    /**
     * @brief Returns current config snapshot.
     * @returns Copy of stored `ServerConfig`.
     */
    ServerConfig get() const {
        std::lock_guard<std::mutex> lk(mtx);
        return cfg;
    }

    /**
     * @brief Replaces current config.
     * @param new_cfg New configuration snapshot.
     */
    void update(const ServerConfig& new_cfg) {
        std::lock_guard<std::mutex> lk(mtx);
        cfg = new_cfg;
    }

private:
    mutable std::mutex mtx;  ///< Guards access to `cfg`.
    ServerConfig cfg;        ///< Runtime server configuration.
};

}
