add_library(web_htop_common STATIC)

target_sources(web_htop_common PRIVATE
    src/common/json/parser.cpp
    src/common/json/utils.cpp

    src/common/models/cpu_metrics.cpp
    src/common/models/disk_metrics.cpp
    src/common/models/loadavg_metrics.cpp
    src/common/models/memory_metrics.cpp
    src/common/models/network_metrics.cpp
    src/common/models/process_metrics.cpp
    src/common/models/system_snapshot.cpp
    src/common/models/telemetry.cpp

    src/protocol.cpp
)

target_include_directories(web_htop_common
    PUBLIC
        include
)

target_link_libraries(web_htop_common
    PUBLIC
        Threads::Threads
        web_htop_options
)
