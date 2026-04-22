add_library(web_htop_server_core STATIC
    src/app/server_app.cpp
    src/app/server_context.cpp
    src/config/config_manager.cpp
    src/state/shared_state.cpp
    src/state/process_history.cpp
    src/collectors/metrics_collector.cpp
    src/collectors/cpu_collector.cpp
    src/collectors/memory_collector.cpp
    src/collectors/disk_collector.cpp
    src/collectors/network_collector.cpp
    src/collectors/process_collector.cpp
    src/collectors/loadavg_collector.cpp
    src/collectors/collector_utils.cpp
    src/transport/streaming_server.cpp
    src/transport/client_session.cpp
    src/transport/broadcaster.cpp
    src/transport/tcp_message_framer.cpp
    src/http/http_server.cpp
    src/http/http_handlers.cpp
    src/http/http_router.cpp
    src/http/dto_mapper.cpp
    src/control/process_controller.cpp
    src/system/proc_reader.cpp
    src/system/filesystem_reader.cpp
    src/system/linux_parser.cpp
)

# Includes use style `#include "server/..."` -> base path is server/include.
# `common/...` comes from linked target `web_htop_common` (PUBLIC include dir).
target_include_directories(web_htop_server_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(web_htop_server_core
    PUBLIC
        web_htop_common
        Threads::Threads
)

if(WEB_HTOP_BUILD_APPS)
    add_executable(web_htop_server
        src/main.cpp
    )

    target_link_libraries(web_htop_server
        PRIVATE
            web_htop_server_core
    )
endif()