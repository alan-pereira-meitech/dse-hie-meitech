cmake_minimum_required(VERSION 3.16)
project(dsejet_cpp_sdk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_EXAMPLES "Build example binaries" ON)
option(BUILD_TESTS "Build test suite" ON)

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

find_package(nlohmann_json 3.2.0 QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

add_library(dsejet
    src/logging.cpp
    src/dsejet_ws_client.cpp
    src/dsejet_sdk.cpp
)

target_include_directories(dsejet PUBLIC include src)

target_link_libraries(dsejet
    PUBLIC
        Boost::system
        Boost::thread
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
)

target_compile_definitions(dsejet
    PRIVATE
        BOOST_ASIO_NO_DEPRECATED
)

if(BUILD_EXAMPLES)
    add_executable(read_stream examples/read_stream.cpp)
    add_executable(read_poll examples/read_poll.cpp)
    add_executable(tare_zero examples/tare_zero.cpp)
    add_executable(set_filter examples/set_filter.cpp)

    foreach(example IN ITEMS read_stream read_poll tare_zero set_filter)
        target_link_libraries(${example} PRIVATE dsejet)
    endforeach()

    add_custom_target(dsejet_examples
        DEPENDS read_stream read_poll tare_zero set_filter)
endif()

if(BUILD_TESTS)
    include(CTest)
    enable_testing()

    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
        )
        FetchContent_MakeAvailable(googletest)
    endif()

    add_executable(dsejet_tests
        tests/unit_message_codec.cpp
        tests/unit_reconnect.cpp
        tests/integration_end_to_end.cpp
    )

    target_link_libraries(dsejet_tests PRIVATE dsejet GTest::gtest GTest::gtest_main)

    add_test(NAME dsejet_tests COMMAND dsejet_tests)
endif()

