#pragma once

#include <chrono>

namespace web_htop::client {

class ReconnectPolicy {
public:
    [[nodiscard]] std::chrono::milliseconds NextDelay(unsigned int attempt) const;
};

}  // namespace web_htop::client
