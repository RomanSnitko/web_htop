#pragma once

#include <string_view>

namespace web_htop::client::ui::theme {

inline constexpr std::string_view kReset = "\033[0m";
inline constexpr std::string_view kAccent = "\033[36m";
inline constexpr std::string_view kSuccess = "\033[32m";
inline constexpr std::string_view kWarning = "\033[33m";
inline constexpr std::string_view kError = "\033[31m";

}  // namespace web_htop::client::ui::theme
