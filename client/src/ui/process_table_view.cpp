#include "client/ui/process_table_view.hpp"

#include "client/filters/process_filter.hpp"
#include "client/filters/process_sort.hpp"
#include "client/ui/formatters.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace web_htop::client::ui {

std::string RenderProcessTable(const web_htop::models::ProcessMetrics& metrics, std::size_t max_rows) {
    std::vector<web_htop::models::ProcessInfo> processes =
        filters::FilterByName(metrics.processes, "");
    filters::SortProcesses(processes, filters::ProcessSortField::Cpu, true);

    if (processes.size() > max_rows) {
        processes.resize(max_rows);
    }

    std::ostringstream out;
    out << "PID      CPU%    MEM       STATE  NAME\n";
    out << "--------------------------------------------\n";

    for (const auto& process : processes) {
        out << std::left << std::setw(8) << process.pid << std::setw(8) << std::fixed
            << std::setprecision(1) << process.cpu_percent << std::setw(10)
            << formatters::FormatBytes(process.memory_bytes) << std::setw(7)
            << formatters::FormatProcessState(process.state) << process.name << '\n';
    }

    return out.str();
}

}  // namespace web_htop::client::ui
