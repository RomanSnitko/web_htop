#pragma once

#include "common/models/process_metrics.hpp"

#include <vector>

namespace web_htop::client::filters {

enum class ProcessSortField {
    Cpu,
    Memory,
    Pid,
    Name,
};

void SortProcesses(std::vector<web_htop::models::ProcessInfo>& processes, ProcessSortField field,
                   bool descending);

}  // namespace web_htop::client::filters
