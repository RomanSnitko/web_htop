add_library(web_htop_server_core STATIC)

target_sources(web_htop_server_core PRIVATE
    src/app/server_app.cpp
    src/collectors/linux_samples.cpp
    src/collectors/metrics_collector.cpp
    src/config/server_config.cpp
    src/http/http_request.cpp
    src/state/shared_state.cpp
    src/system/linux_source.cpp
    src/transport/reactor.cpp
)

target_include_directories(web_htop_server_core
    PUBLIC
        include
)

target_link_libraries(web_htop_server_core
    PUBLIC
        web_htop_common
)

if(WEB_HTOP_BUILD_APPS)
    add_executable(web_htop_server src/main.cpp)
    target_link_libraries(web_htop_server PRIVATE web_htop_server_core)
    install(TARGETS web_htop_server RUNTIME DESTINATION bin)
endif()
