/** @file server/http/http_request.hpp
 *  @brief Strict GET-only HTTP/1.x subset; one request per connection.
 */
#pragma once
#include <string>
#include <string_view>

namespace web_htop::server
{
struct HttpRequest
{
    unsigned status{200};
    std::string target;
    std::string path;
    std::string query;
};

[[nodiscard]] HttpRequest ParseHttpRequest(std::string_view text);
[[nodiscard]] std::string HttpResponse(unsigned status, std::string_view body,
                                       std::string_view type = "application/json");
} // namespace web_htop::server
