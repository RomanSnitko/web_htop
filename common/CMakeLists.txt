add_library(web_htop_common STATIC
    src/common/json/utils.cpp
    src/common/json/parser.cpp
    src/common/models/cpu_metrics.cpp
    src/common/models/memory_metrics.cpp
    src/common/models/disk_metrics.cpp
    src/common/models/network_metrics.cpp
    src/common/models/process_metrics.cpp
    src/common/models/loadavg_metrics.cpp
    src/common/models/system_snapshot.cpp
)

target_include_directories(web_htop_common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)