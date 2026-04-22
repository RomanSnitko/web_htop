/**
 * @file server/src/http/http_router.cpp
 * 
 * @author Roman Snitko
 * @date 2026-04-16
 * 
 * @brief Implementation unit for `server/http/http_router.hpp`.
 * @details Routes parsed requests to concrete handler methods.
 */

#include "server/http/http_router.hpp"
#include "server/http/http_handlers.hpp"

namespace web_htop::server {

HttpRouter::HttpRouter(HttpHandlers* handlers) : handlers_(handlers) {}

HttpResponse HttpRouter::Route(const HttpRequest& request) const {
    if (handlers_ == nullptr) {
        return HttpResponse{500, "Internal Server Error", "application/json",
                            R"({"error":"router_not_initialized"})", true};
    }

    if (request.method != "GET") {
        return handlers_->HandleMethodNotAllowed();
    }

    if (request.path == "/health") {
        return handlers_->HandleHealth();
    }
    if (request.path == "/metrics") {
        return handlers_->HandleMetrics();
    }
    if (request.path == "/processes") {
        return handlers_->HandleProcesses();
    }

    return handlers_->HandleNotFound();
}

}
