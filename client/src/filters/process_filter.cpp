#include "client/filters/process_filter.hpp"

#include <algorithm>
#include <cctype>

namespace web_htop::client::filters {

namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

}  // namespace

std::vector<web_htop::models::ProcessInfo> FilterByName(
    const std::vector<web_htop::models::ProcessInfo>& processes, const std::string& query) {
    if (query.empty()) {
        return processes;
    }

    const std::string query_lower = ToLower(query);
    std::vector<web_htop::models::ProcessInfo> result;
    result.reserve(processes.size());
    for (const auto& process : processes) {
        if (ToLower(process.name).find(query_lower) != std::string::npos) {
            result.push_back(process);
        }
    }
    return result;
}

}  // namespace web_htop::client::filters
