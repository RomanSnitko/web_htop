#include "client/net/reconnect_policy.hpp"

#include <algorithm>

namespace web_htop::client {

std::chrono::milliseconds ReconnectPolicy::NextDelay(unsigned int attempt) const {
    const unsigned int exponent = std::min(attempt, 5U);
    const auto delay_ms = static_cast<int>(500U * (1U << exponent));
    return std::chrono::milliseconds(delay_ms);
}

}  // namespace web_htop::client
