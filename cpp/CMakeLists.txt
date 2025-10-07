cmake_minimum_required(VERSION 3.16)
project(hbm_automation_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(hbm_automation
    src/base_wt_device.cpp
    src/jet_bus_command.cpp
    src/jet_bus_connection.cpp
    src/jet_process_data.cpp
    src/wtx_jet.cpp
)

target_include_directories(hbm_automation
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hbm_automation
    PUBLIC
        pthread
)

add_executable(wtx_example examples/wtx_example.cpp)
target_link_libraries(wtx_example PRIVATE hbm_automation)

add_executable(test_wtx tests/test_wtx.cpp)
target_link_libraries(test_wtx PRIVATE hbm_automation)

enable_testing()
add_test(NAME hbm_wtx_test COMMAND test_wtx)
