if(NOT WEB_HTOP_BUILD_TESTS)
    return()
endif()

include(FetchContent)
enable_testing()
include(GoogleTest)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(web_htop_test_json_parser
    common/test_json_parser.cpp
)

target_link_libraries(web_htop_test_json_parser
    PRIVATE
        web_htop_common
        GTest::gtest_main
)

gtest_discover_tests(web_htop_test_json_parser)

add_executable(web_htop_test_serialization
    common/test_serialization.cpp
)

target_link_libraries(web_htop_test_serialization
    PRIVATE
        web_htop_common
        GTest::gtest_main
)

gtest_discover_tests(web_htop_test_serialization)

add_executable(web_htop_test_http_router
    server/test_http_router.cpp
)

target_link_libraries(web_htop_test_http_router
    PRIVATE
        web_htop_server_core
        GTest::gtest_main
)

gtest_discover_tests(web_htop_test_http_router)
