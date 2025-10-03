cmake_minimum_required(VERSION 3.20)
project(dse_jet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(dse STATIC
    src/jet_connection.cpp
    src/dse_jet.cpp
)

target_include_directories(dse PUBLIC include)

target_compile_options(dse PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic>
)

add_executable(configure_filters samples/configure_filters.cpp)
target_link_libraries(configure_filters PRIVATE dse)

if (WIN32)
    target_link_libraries(dse PRIVATE ws2_32)
endif()
