#include "client/filters/process_sort.hpp"

#include <algorithm>

namespace web_htop::client::filters {

void SortProcesses(std::vector<web_htop::models::ProcessInfo>& processes, ProcessSortField field,
                   bool descending) {
    auto compare = [field](const web_htop::models::ProcessInfo& lhs,
                           const web_htop::models::ProcessInfo& rhs) {
        switch (field) {
            case ProcessSortField::Cpu:
                return lhs.cpu_percent < rhs.cpu_percent;
            case ProcessSortField::Memory:
                return lhs.memory_bytes < rhs.memory_bytes;
            case ProcessSortField::Pid:
                return lhs.pid < rhs.pid;
            case ProcessSortField::Name:
                return lhs.name < rhs.name;
        }
        return false;
    };

    if (descending) {
        std::sort(processes.begin(), processes.end(),
                  [&](const auto& lhs, const auto& rhs) { return compare(rhs, lhs); });
    } else {
        std::sort(processes.begin(), processes.end(), compare);
    }
}

}  // namespace web_htop::client::filters
