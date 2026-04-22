/**
 * @file server/include/server/http/http_handlers.hpp
 * 
 * @author Roman Snitko
 * @date 2026-04-16
 * 
 * @brief Http handlers interface declarations.
 * @details Uses SharedState to build JSON responses for HTTP API routes.
 */

#pragma once

#include "server/http/http_router.hpp"

namespace web_htop::server {

class SharedState;
class DtoMapper;

class HttpHandlers {
public:
    HttpHandlers(SharedState* state, DtoMapper* mapper);

    [[nodiscard]] HttpResponse HandleHealth() const;
    [[nodiscard]] HttpResponse HandleMetrics() const;
    [[nodiscard]] HttpResponse HandleProcesses() const;
    [[nodiscard]] HttpResponse HandleNotFound() const;
    [[nodiscard]] HttpResponse HandleMethodNotAllowed() const;

private:
    SharedState* state_{nullptr};  ///< Non-owning shared runtime state.
    DtoMapper* mapper_{nullptr};   ///< Non-owning DTO serializer helper.
};

}
