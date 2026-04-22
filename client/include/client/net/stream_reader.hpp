#pragma once

#include "common/models/system_snapshot.hpp"

namespace web_htop::client {

class ServerConnection;

class StreamReader {
public:
    explicit StreamReader(ServerConnection* connection);

    [[nodiscard]] bool ReadSnapshot(web_htop::models::SystemSnapshot& snapshot) const;

private:
    [[nodiscard]] bool read_exact(int fd, char* data, std::size_t size) const;

    ServerConnection* connection_{nullptr};  ///< Non-owning connection reference.
};

}  // namespace web_htop::client
