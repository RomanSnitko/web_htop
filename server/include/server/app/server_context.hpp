/**
 * @file server/include/server/app/server_context.hpp
 *
 * @author Roman Snitko
 * @date 2026-04-16
 *
 * @brief Shared server context declarations.
 * @details Placeholder for app-wide state grouping used by server subsystems.
 */

#pragma once

#include <atomic>

namespace web_htop::server {
/**
 * @todo Define unified server context fields used across modules.
 */

struct SharedState {
	std::atomic<bool> running{false};  ///< Server running flag.
};

}

