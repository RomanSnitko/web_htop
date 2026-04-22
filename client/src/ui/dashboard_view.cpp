#include "client/ui/dashboard_view.hpp"

#include "client/ui/formatters.hpp"

#include <sstream>

namespace web_htop::client::ui {

std::string RenderDashboard(const web_htop::models::SystemSnapshot& snapshot) {
    std::ostringstream out;
    out << "CPU: " << formatters::FormatPercentage(snapshot.cpu.total_usage_percent)
        << "  Freq: " << snapshot.cpu.frequency_mhz << " MHz\n";
    out << "RAM: " << formatters::FormatBytes(snapshot.memory.used_bytes) << " / "
        << formatters::FormatBytes(snapshot.memory.total_bytes) << " ("
        << formatters::FormatPercentage(snapshot.memory.used_percent) << ")\n";
    out << "Disk: " << formatters::FormatBytes(snapshot.disk.used_bytes) << " / "
        << formatters::FormatBytes(snapshot.disk.total_bytes) << " ("
        << formatters::FormatPercentage(snapshot.disk.used_percent) << ")\n";
    out << "Network: RX " << snapshot.network.rx_kbps << " KB/s, TX " << snapshot.network.tx_kbps
        << " KB/s\n";
    out << "Loadavg: " << snapshot.loadavg.load_1m << " " << snapshot.loadavg.load_5m << " "
        << snapshot.loadavg.load_15m << "\n";
    return out.str();
}

}  // namespace web_htop::client::ui
