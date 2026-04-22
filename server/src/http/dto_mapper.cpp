/**
 * @file server/src/http/dto_mapper.cpp
 * 
 * @author Roman Snitko
 * @date 2026-04-16
 * 
 * @brief Implementation unit for `server/http/dto_mapper.hpp`.
 * @details Maps common models to compact JSON strings.
 */

#include "server/http/dto_mapper.hpp"

namespace web_htop::server {

std::string DtoMapper::MapSnapshot(const web_htop::models::SystemSnapshot& snapshot) const {
    return snapshot.ToJson().ToString();
}

std::string DtoMapper::MapProcesses(const web_htop::models::ProcessMetrics& processes) const {
    return processes.ToJson().ToString();
}

}
