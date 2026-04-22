/**
 * @file server/include/server/http/http_router.hpp
 * 
 * @author Roman Snitko
 * @date 2026-04-16
 * 
 * @brief Http router interface declarations.
 * @details Owns route table mapping HTTP method/path to concrete handlers.
 */

#pragma once

#include <string>

namespace web_htop::server {

class HttpHandlers;

/**
 * @brief Minimal parsed HTTP request.
 */
struct HttpRequest {
    std::string method{};
    std::string path{};
    std::string version{};
};

/**
 * @brief HTTP response DTO used by handlers/router/server.
 */
struct HttpResponse {
    int status_code{200};
    std::string status_text{"OK"};
    std::string content_type{"application/json"};
    std::string body{};
    bool close_connection{true};
};

class HttpRouter {
public:
    explicit HttpRouter(HttpHandlers* handlers);

    [[nodiscard]] HttpResponse Route(const HttpRequest& request) const;

private:
    HttpHandlers* handlers_{nullptr};  ///< Non-owning handlers facade.
};

}
