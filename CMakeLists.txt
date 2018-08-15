cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)
project(PumexAppProject LANGUAGES CXX VERSION 00.00.01)

set(BUILD_SHARED_LIBS OFF)

execute_process(COMMAND git submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(PumexApp src/main.cpp)
add_subdirectory(external/pumex)

target_link_libraries(PumexApp PRIVATE pumexlib)
get_target_property(linkLibs PumexApp LINK_LIBRARIES)
message("PumexApp link libs: ${linkLibs}")
get_target_property(pumexLinkLibs pumexlib INTERFACE_LINK_LIBRARIES)
message("pumexlib interface link libs: ${pumexLinkedLibs}")
get_target_property(vulkanIncludes vulkanlib INTERFACE_INCLUDE_DIRECTORIES)
message("vulkanlib interface includes: ${vulkanIncludes}")