cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)
project(PumexAppProject LANGUAGES CXX VERSION 00.00.01)

set(BUILD_SHARED_LIBS OFF)

execute_process(COMMAND git submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(PumexApp src/main.cpp)
add_subdirectory(external/pumex)

set(ShaderNames 
    shaders/3D/shader.vert
    shaders/3D/shader.frag)
process_shaders(
    ${CMAKE_CURRENT_LIST_DIR}
    ShaderNames
    InputShaders
    OutputShaders)
add_custom_target(shaderTarget
    DEPENDS ${OutputShaders}
    SOURCES ${InputShaders})
add_custom_command(
    TARGET shaderTarget
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        "${CMAKE_CURRENT_BINARY_DIR}/shaders")

target_link_libraries(PumexApp PRIVATE pumexlib)
add_dependencies(PumexApp shaderTarget)