#include "client/ui/formatters.hpp"

#include <array>
#include <iomanip>
#include <sstream>

namespace web_htop::client::ui::formatters {

std::string FormatPercentage(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value << '%';
    return out.str();
}

std::string FormatBytes(web_htop::Bytes bytes) {
    constexpr std::array<const char*, 5> units = {"B", "KiB", "MiB", "GiB", "TiB"};
    double value = static_cast<double>(bytes);
    std::size_t unit = 0;
    while (value >= 1024.0 && unit + 1 < units.size()) {
        value /= 1024.0;
        ++unit;
    }

    std::ostringstream out;
    out << std::fixed << std::setprecision(unit == 0 ? 0 : 1) << value << ' ' << units[unit];
    return out.str();
}

std::string FormatProcessState(web_htop::ProcessState state) {
    return std::string(1, static_cast<char>(state));
}

}  // namespace web_htop::client::ui::formatters
