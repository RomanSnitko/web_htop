#pragma once

#include "common/models/process_metrics.hpp"

#include <string>

namespace web_htop::client::ui {

[[nodiscard]] std::string RenderProcessTable(const web_htop::models::ProcessMetrics& metrics,
                                             std::size_t max_rows);

}  // namespace web_htop::client::ui
