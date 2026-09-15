#include "server/http/http_request.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <map>
#include <sstream>

namespace web_htop::server
{
HttpRequest ParseHttpRequest(std::string_view text)
{
    auto bad = [](unsigned status = 400)
    {
        return HttpRequest{status, {}};
    };
    auto end = text.find("\r\n\r\n");

    if (end == std::string_view::npos || end + 4 != text.size())
    {
        return bad();
    }
    if (text.size() > 8192)
    {
        return bad(431);
    }
    auto first = text.find("\r\n");
    std::istringstream line(std::string(text.substr(0, first)));
    std::string method, target, version, extra;

    if (!(line >> method >> target >> version) || (line >> extra))
    {
        return bad();
    }
    if (method != "GET")
    {
        return bad(405);
    }
    if (version != "HTTP/1.1" && version != "HTTP/1.0")
    {
        return bad(505);
    }
    if (target.empty() || target.front() != '/')
    {
        return bad();
    }
    for (unsigned char c : target)
    {
        if (c <= 32 || c >= 127)
        {
            return bad();
        }
    }
    std::map<std::string, std::string> headers;

    for (auto pos = first + 2; pos < end;)
    {
        auto next = text.find("\r\n", pos);

        if (next == std::string_view::npos)
        {
            return bad();
        }
        auto row = text.substr(pos, next - pos);
        auto colon = row.find(':');

        if (colon == std::string_view::npos || colon == 0)
        {
            return bad();
        }
        std::string key(row.substr(0, colon));

        for (unsigned char c : key)
        {
            if (!std::isalnum(c) &&
                std::string_view("!#$%&'*+-.^_`|~").find(static_cast<char>(c)) ==
                    std::string_view::npos)
            {
                return bad();
            }
        }
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c)
                       {
                           return static_cast<char>(std::tolower(c));
                       });
        auto value = row.substr(colon + 1);

        while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
        {
            value.remove_prefix(1);
        }
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t'))
        {
            value.remove_suffix(1);
        }
        for (unsigned char c : value)
        {
            if ((c < 32 && c != '\t') || c == 127)
            {
                return bad();
            }
        }
        if (!headers.emplace(key, std::string(value)).second)
        {
            return bad();
        }
        pos = next + 2;
    }
    if (version == "HTTP/1.1" && (!headers.contains("host") || headers["host"].empty()))
    {
        return bad();
    }
    if (headers.contains("transfer-encoding"))
    {
        return bad();
    }
    if (headers.contains("expect"))
    {
        return bad(417);
    }
    if (auto it = headers.find("content-length"); it != headers.end())
    {
        unsigned n{};
        auto const& s = it->second;
        auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), n);

        if (ec != std::errc{} || p != s.data() + s.size() || n != 0)
        {
            return bad();
        }
    }
    auto q = target.find('?');
    std::string path = (q == std::string::npos) ? target : target.substr(0, q);
    std::string query = (q == std::string::npos) ? "" : target.substr(q + 1);
    return {200, std::move(target), std::move(path), std::move(query)};
}

std::string HttpResponse(unsigned status, std::string_view body, std::string_view type)
{
    std::string reason;

    switch (status)
    {
    case 200:
        reason = "OK";
        break;
    case 400:
        reason = "Bad Request";
        break;
    case 404:
        reason = "Not Found";
        break;
    case 405:
        reason = "Method Not Allowed";
        break;
    case 417:
        reason = "Expectation Failed";
        break;
    case 431:
        reason = "Request Header Fields Too Large";
        break;
    case 503:
        reason = "Service Unavailable";
        break;
    case 505:
        reason = "HTTP Version Not Supported";
        break;
    default:
        reason = "Internal Server Error";
        break;
    }
    std::string response = "HTTP/1.1 " + std::to_string(status) + " " + reason +
                           "\r\nContent-Type: " + std::string(type) +
                           "\r\nContent-Length: " + std::to_string(body.size()) +
                           "\r\nConnection: close\r\nCache-Control: no-store\r\n";

    if (status == 405)
    {
        response += "Allow: GET\r\n";
    }
    response += "\r\n";
    response += body;
    return response;
}
} // namespace web_htop::server
