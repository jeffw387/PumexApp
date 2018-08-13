cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)
project(PumexAppProject LANGUAGES CXX VERSION 00.00.01)

execute_process(COMMAND git submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(PumexApp /src/main.cpp)
add_subdirectory(external/pumex)

target_link_libraries(PumexApp PRIVATE pumexlib)