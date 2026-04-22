#include "client/ui/status_bar_view.hpp"

#include <sstream>

namespace web_htop::client::ui {

std::string RenderStatusBar(bool connected, const std::string& message) {
    std::ostringstream out;
    out << "[connection: " << (connected ? "online" : "offline") << "] " << message;
    return out.str();
}

}  // namespace web_htop::client::ui
