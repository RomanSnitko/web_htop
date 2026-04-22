#include <gtest/gtest.h>

#include "server/http/dto_mapper.hpp"
#include "server/http/http_handlers.hpp"
#include "server/http/http_router.hpp"
#include "server/state/shared_state.hpp"

namespace {

TEST(HttpRouterTest, ReturnsHealthForKnownEndpoint) {
    web_htop::server::SharedState state;
    state.set_running(true);

    web_htop::server::DtoMapper mapper;
    web_htop::server::HttpHandlers handlers(&state, &mapper);
    web_htop::server::HttpRouter router(&handlers);

    web_htop::server::HttpRequest request{"GET", "/health", "HTTP/1.1"};
    const auto response = router.Route(request);

    EXPECT_EQ(response.status_code, 200);
    EXPECT_NE(response.body.find("\"status\":\"ok\""), std::string::npos);
}

TEST(HttpRouterTest, ReturnsNotFoundForUnknownEndpoint) {
    web_htop::server::SharedState state;
    web_htop::server::DtoMapper mapper;
    web_htop::server::HttpHandlers handlers(&state, &mapper);
    web_htop::server::HttpRouter router(&handlers);

    web_htop::server::HttpRequest request{"GET", "/unknown", "HTTP/1.1"};
    const auto response = router.Route(request);

    EXPECT_EQ(response.status_code, 404);
}

}  // namespace
