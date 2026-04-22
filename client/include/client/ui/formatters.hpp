#pragma once

#include "common/enums.hpp"
#include "common/types.hpp"

#include <string>

namespace web_htop::client::ui::formatters {

[[nodiscard]] std::string FormatPercentage(double value);
[[nodiscard]] std::string FormatBytes(web_htop::Bytes bytes);
[[nodiscard]] std::string FormatProcessState(web_htop::ProcessState state);

}  // namespace web_htop::client::ui::formatters
