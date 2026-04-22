#pragma once

#include <string>

namespace web_htop::client::ui {

[[nodiscard]] std::string RenderStatusBar(bool connected, const std::string& message);

}  // namespace web_htop::client::ui
