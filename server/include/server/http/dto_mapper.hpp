/**
 * @file server/include/server/http/dto_mapper.hpp
 * 
 * @author Roman Snitko
 * @date 2026-04-16
 * 
 * @brief Dto mapper interface for http layer.
 * @details Converts runtime models to JSON payload strings.
 */

#pragma once

#include "common/models/process_metrics.hpp"
#include "common/models/system_snapshot.hpp"

#include <string>

namespace web_htop::server {

class DtoMapper {
public:
    [[nodiscard]] std::string MapSnapshot(const web_htop::models::SystemSnapshot& snapshot) const;
    [[nodiscard]] std::string MapProcesses(const web_htop::models::ProcessMetrics& processes) const;
};

}
