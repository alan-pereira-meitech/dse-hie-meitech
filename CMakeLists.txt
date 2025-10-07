cmake_minimum_required(VERSION 3.16)
project(dse_jetbus LANGUAGES CXX)

option(ENABLE_RECONNECT "Enable automatic reconnection in JetBusClient" ON)
option(LOG_LEVEL_TRACE "Enable verbose logging" OFF)
option(PROCESS_DATA_INTERVAL_MS "Default process data interval in milliseconds" 200)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

find_package(Threads REQUIRED)
find_package(Boost 1.74 QUIET COMPONENTS system)
if (NOT Boost_FOUND)
    message(STATUS "Boost not found on system - downloading headers")
    set(BOOST_VERSION 1.83.0)
    string(REPLACE "." "_" BOOST_VERSION_TAG ${BOOST_VERSION})
    set(BOOST_ARCHIVE ${CMAKE_BINARY_DIR}/boost_${BOOST_VERSION_TAG}.tar.gz)
    if (NOT EXISTS ${BOOST_ARCHIVE})
        file(DOWNLOAD
            https://archives.boost.io/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_TAG}.tar.gz
            ${BOOST_ARCHIVE}
            SHOW_PROGRESS
        )
    endif()
    set(JETBUS_BOOST_INCLUDE_DIR ${CMAKE_BINARY_DIR}/boost_${BOOST_VERSION_TAG})
    if (NOT EXISTS ${JETBUS_BOOST_INCLUDE_DIR})
        execute_process(COMMAND tar -xzf ${BOOST_ARCHIVE} WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()
endif()

find_package(nlohmann_json QUIET)
if (nlohmann_json_FOUND)
    get_target_property(NLOHMANN_JSON_INCLUDE_DIR nlohmann_json::nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
else()
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
    )
    FetchContent_MakeAvailable(nlohmann_json)
    set(NLOHMANN_JSON_INCLUDE_DIR ${nlohmann_json_SOURCE_DIR}/include)
endif()

add_library(jetbus_core
    src/jetbus/client.cpp
    src/jetbus/commands.cpp
    src/jetbus/process_data.cpp
)

target_include_directories(jetbus_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${NLOHMANN_JSON_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(jetbus_core
    PRIVATE
        Threads::Threads
)

if (Boost_FOUND)
    target_link_libraries(jetbus_core PRIVATE Boost::system)
else()
    target_compile_definitions(jetbus_core PRIVATE BOOST_ERROR_CODE_HEADER_ONLY BOOST_SYSTEM_NO_DEPRECATED)
    if (DEFINED JETBUS_BOOST_INCLUDE_DIR)
        target_include_directories(jetbus_core PRIVATE ${JETBUS_BOOST_INCLUDE_DIR})
    endif()
endif()

add_library(dse_device
    src/dse/device.cpp
)

add_dependencies(dse_device jetbus_core)

target_link_libraries(dse_device
    PUBLIC
        jetbus_core
)

target_include_directories(dse_device
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_include_directories(dse_device PRIVATE ${NLOHMANN_JSON_INCLUDE_DIR})



add_executable(dse_stream_1s
    examples/stream_every_second.cpp
)

target_link_libraries(dse_stream_1s
    PRIVATE
        dse_device
)

install(TARGETS jetbus_core dse_device dse_stream_1s
    EXPORT dse_jetbusTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT dse_jetbusTargets
    FILE dse_jetbusTargets.cmake
    NAMESPACE dse::
    DESTINATION lib/cmake/dse_jetbus
)

enable_testing()

FetchContent_Declare(
    Catch2
    URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.4.0.tar.gz
)
FetchContent_MakeAvailable(Catch2)

add_executable(jetbus_tests
    tests/measurement_utils_test.cpp
    tests/process_data_test.cpp
)

target_link_libraries(jetbus_tests
    PRIVATE
        dse_device
        Catch2::Catch2WithMain
)

add_test(NAME jetbus_tests COMMAND jetbus_tests)
