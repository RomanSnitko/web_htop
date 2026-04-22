#pragma once

#include "client/state/client_state.hpp"

namespace web_htop::client {

class TerminalUi {
public:
    void Render(const ClientState& state) const;
};

}  // namespace web_htop::client
