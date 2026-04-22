/**
 * @file server/src/http/http_handlers.cpp
 * 
 * @author Roman Snitko
 * @date 2026-04-16
 * 
 * @brief Implementation unit for `server/http/http_handlers.hpp`.
 * @details Contains concrete logic for HTTP API endpoints.
 */

#include "server/http/http_handlers.hpp"

#include "server/http/dto_mapper.hpp"
#include "server/state/shared_state.hpp"

namespace web_htop::server {

HttpHandlers::HttpHandlers(SharedState* state, DtoMapper* mapper)
    : state_(state), mapper_(mapper) {}

HttpResponse HttpHandlers::HandleHealth() const {
    const bool running = (state_ != nullptr) ? state_->is_running() : false;
    const char* status = running ? "ok" : "starting";
    return HttpResponse{200, "OK", "application/json",
                        std::string("{\"status\":\"") + status + "\"}", true};
}

HttpResponse HttpHandlers::HandleMetrics() const {
    if (state_ == nullptr || mapper_ == nullptr) {
        return HttpResponse{500, "Internal Server Error", "application/json",
                            R"({"error":"state_unavailable"})", true};
    }

    const auto snapshot = state_->get_system_snapshot();
    return HttpResponse{200, "OK", "application/json", mapper_->MapSnapshot(snapshot), true};
}

HttpResponse HttpHandlers::HandleProcesses() const {
    if (state_ == nullptr || mapper_ == nullptr) {
        return HttpResponse{500, "Internal Server Error", "application/json",
                            R"({"error":"state_unavailable"})", true};
    }

    const auto snapshot = state_->get_system_snapshot();
    return HttpResponse{200, "OK", "application/json", mapper_->MapProcesses(snapshot.process),
                        true};
}

HttpResponse HttpHandlers::HandleNotFound() const {
    return HttpResponse{404, "Not Found", "application/json", R"({"error":"not_found"})", true};
}

HttpResponse HttpHandlers::HandleMethodNotAllowed() const {
    return HttpResponse{405, "Method Not Allowed", "application/json",
                        R"({"error":"method_not_allowed"})", true};
}

}
