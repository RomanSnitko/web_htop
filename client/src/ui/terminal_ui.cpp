#include "client/ui/terminal_ui.hpp"

#include "client/ui/dashboard_view.hpp"
#include "client/ui/help_view.hpp"
#include "client/ui/process_table_view.hpp"
#include "client/ui/status_bar_view.hpp"

#include <iostream>

namespace web_htop::client {

void TerminalUi::Render(const ClientState& state) const {
    const auto snapshot = state.GetSnapshot();
    const bool connected = state.IsConnected();
    const std::string message = state.GetConnectionMessage();

    std::cout << "\033[2J\033[H";
    std::cout << "web_htop client\n";
    std::cout << ui::RenderStatusBar(connected, message) << "\n\n";
    std::cout << ui::RenderDashboard(snapshot) << "\n";
    std::cout << ui::RenderProcessTable(snapshot.process, 12) << "\n";
    std::cout << ui::RenderHelp() << "\n";
    std::cout.flush();
}

}  // namespace web_htop::client
