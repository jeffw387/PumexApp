cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)
project(PumexAppProject LANGUAGES CXX VERSION 00.00.01)

set(BUILD_SHARED_LIBS OFF)

execute_process(COMMAND git submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(PumexAppSources ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
add_executable(PumexApp ${PumexAppSources})
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

add_custom_command(TARGET PumexApp
    POST_BUILD
    COMMAND clang-format -i -style=file ${PumexAppSources})

function(copyToDir InputDir InputNames OutputDir TargetName)
    message("copyToDir: InputDir=${InputDir}, " 
        "InputNames=${${InputNames}}, "
        "OutputDir=${OutputDir}, "
        "TargetName=${TargetName}")
    set(inFiles)
    set(outFiles)
    if(NOT EXISTS "${OutputDir}/")
        file(MAKE_DIRECTORY "${OutputDir}")
    endif()
    foreach(InputName ${${InputNames}})
        set(outFile "${OutputDir}/${InputName}")
        set(inFile "${InputDir}/${InputName}")
        message("copyToDir: inFile=${inFile}")
        message("copyToDir: outFile=${outFile}")
        list(APPEND inFiles ${inFile})
        list(APPEND outFiles ${outFile})
        add_custom_command(OUTPUT ${outFile}
            DEPENDS ${inFile}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${inFile} ${outFile})
    endforeach()
    message("copyToDir: adding custom target ${TargetName}")
    add_custom_target(
        ${TargetName}
        DEPENDS ${outFiles}
        SOURCES ${inFiles})
endfunction()

list(APPEND cubeModelFiles cube.blend cube.gltf cube.bin)
list(APPEND cylinderModelFiles cylinder.blend cylinder.gltf cylinder.bin)
list(APPEND icosphereSub2ModelFiles icosphereSub2.blend icosphereSub2.gltf icosphereSub2.bin)
list(APPEND pentagonModelFiles pentagon.blend pentagon.gltf pentagon.bin)
list(APPEND triangleModelFiles triangle.blend triangle.gltf triangle.bin)

add_custom_target(
    CubeModel
    DEPENDS cubeModelFiles)
add_custom_target(
    CylinderModel
    DEPENDS cylinderModelFiles)
add_custom_target(
    IcosphereSub2Model
    DEPENDS icosphereSub2ModelFiles)
add_custom_target(
    PentagonModel
    DEPENDS pentagonModelFiles)
add_custom_target(
    TriangleModel
    DEPENDS triangleModelFiles)

list(APPEND modelFiles
    ${cubeModelFiles}
    ${cylinderModelFiles}
    ${icosphereSub2ModelFiles}
    ${pentagonModelFiles}
    ${triangleModelFiles})
copyToDir(
    "${CMAKE_CURRENT_SOURCE_DIR}/content/models"
    modelFiles
    "${CMAKE_CURRENT_BINARY_DIR}/content/models"
    ModelCopyTarget)
target_link_libraries(PumexApp PRIVATE pumexlib)
add_dependencies(PumexApp shaderTarget ModelCopyTarget)