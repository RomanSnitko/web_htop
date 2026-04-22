#pragma once

#include "common/models/process_metrics.hpp"
#include "common/models/system_snapshot.hpp"

#include <optional>
#include <string>

namespace web_htop::client {

class HttpControlClient {
public:
    HttpControlClient(std::string host, int port);

    [[nodiscard]] bool FetchMetrics(web_htop::models::SystemSnapshot& snapshot) const;
    [[nodiscard]] bool FetchProcesses(web_htop::models::ProcessMetrics& processes) const;

private:
    [[nodiscard]] std::optional<std::string> PerformGet(const std::string& path) const;

    std::string host_{};
    int port_{8080};
};

}  // namespace web_htop::client
