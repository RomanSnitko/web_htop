add_library(web_htop_client_core STATIC
    src/app/client_app.cpp
    src/app/client_context.cpp
    src/state/client_state.cpp
    src/net/server_connection.cpp
    src/net/reconnect_policy.cpp
    src/net/stream_reader.cpp
    src/http/http_control_client.cpp
    src/ui/terminal_ui.cpp
    src/ui/dashboard_view.cpp
    src/ui/process_table_view.cpp
    src/ui/status_bar_view.cpp
    src/ui/help_view.cpp
    src/ui/theme.cpp
    src/ui/formatters.cpp
    src/filters/process_filter.cpp
    src/filters/process_sort.cpp
)

target_include_directories(web_htop_client_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(web_htop_client_core
    PUBLIC
        web_htop_common
        Threads::Threads
)

if(WEB_HTOP_BUILD_APPS)
    add_executable(web_htop_client src/main.cpp)
    target_link_libraries(web_htop_client PRIVATE web_htop_client_core)
endif()
