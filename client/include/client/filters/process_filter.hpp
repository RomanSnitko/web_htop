#pragma once

#include "common/models/process_metrics.hpp"

#include <string>
#include <vector>

namespace web_htop::client::filters {

[[nodiscard]] std::vector<web_htop::models::ProcessInfo> FilterByName(
    const std::vector<web_htop::models::ProcessInfo>& processes, const std::string& query);

}  // namespace web_htop::client::filters
