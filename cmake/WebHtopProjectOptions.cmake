function(web_htop_configure_project_options)
    set(CMAKE_CXX_STANDARD 20 PARENT_SCOPE)
    set(CMAKE_CXX_STANDARD_REQUIRED ON PARENT_SCOPE)
    set(CMAKE_CXX_EXTENSIONS OFF PARENT_SCOPE)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON PARENT_SCOPE)

    option(WEB_HTOP_BUILD_APPS
        "Build server and client"
        ON
    )
    option(WEB_HTOP_BUILD_TESTS
        "Build deterministic and integration tests"
        ON
    )
    option(WEB_HTOP_BUILD_LEGACY_TESTS
        "Build retained GoogleTest codec tests (requires installed GTest)"
        OFF
    )
    option(WEB_HTOP_BUILD_FUZZERS
        "Build libFuzzer targets (Clang)"
        OFF
    )

    set(WEB_HTOP_SANITIZER "" CACHE STRING
        "Runtime sanitizer: address, undefined or thread"
    )
    set_property(CACHE WEB_HTOP_SANITIZER PROPERTY STRINGS
        "" address undefined thread
    )

    add_library(web_htop_options INTERFACE)
    target_compile_features(web_htop_options INTERFACE cxx_std_20)
    target_compile_options(web_htop_options INTERFACE
        "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wformat=2>"
    )

    if(WEB_HTOP_SANITIZER)
        if(NOT WEB_HTOP_SANITIZER MATCHES "^(address|undefined|thread)$")
            message(FATAL_ERROR
                "WEB_HTOP_SANITIZER must be address, undefined or thread"
            )
        endif()

        target_compile_options(web_htop_options INTERFACE
            -fsanitize=${WEB_HTOP_SANITIZER}
            -fno-omit-frame-pointer
            -g
        )
        target_link_options(web_htop_options INTERFACE
            -fsanitize=${WEB_HTOP_SANITIZER}
        )
    endif()
endfunction()
