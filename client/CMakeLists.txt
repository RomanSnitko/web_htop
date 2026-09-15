if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/app/client_app.cpp")
    add_library(web_htop_client_core STATIC)

    target_sources(web_htop_client_core PRIVATE
        src/app/client_app.cpp
        src/net/stream_connection.cpp
        src/ui/terminal_ui.cpp
    )

    target_include_directories(web_htop_client_core
        PUBLIC
            include
    )

    target_link_libraries(web_htop_client_core
        PUBLIC
            web_htop_common
    )

    if(WEB_HTOP_BUILD_APPS)
        add_executable(web_htop_client src/main.cpp)
        target_link_libraries(web_htop_client PRIVATE web_htop_client_core)
        install(TARGETS web_htop_client RUNTIME DESTINATION bin)
    endif()
endif()
