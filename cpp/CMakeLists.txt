cmake_minimum_required(VERSION 3.16)
project(dse_jet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED COMPONENTS system)

add_library(dse_jet
    src/DseJetConnection.cpp
    src/DseJetProtocolClient.cpp
)

target_include_directories(dse_jet
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(dse_jet PUBLIC Boost::system)

target_compile_definitions(dse_jet PRIVATE BOOST_ASIO_NO_DEPRECATED)

target_compile_options(dse_jet PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/permissive- /W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

install(TARGETS dse_jet
    EXPORT dse_jetTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT dse_jetTargets
    FILE dse_jetTargets.cmake
    NAMESPACE dse::
    DESTINATION lib/cmake/dse_jet)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/dse_jetConfigVersion.cmake"
    VERSION "1.0.0"
    COMPATIBILITY SameMajorVersion)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/dse_jetConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/dse_jetConfig.cmake"
    @ONLY)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/dse_jetConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/dse_jetConfigVersion.cmake"
    DESTINATION lib/cmake/dse_jet)

