#pragma once

#include "common/models/system_snapshot.hpp"

#include <string>

namespace web_htop::client::ui {

[[nodiscard]] std::string RenderDashboard(const web_htop::models::SystemSnapshot& snapshot);

}  // namespace web_htop::client::ui
