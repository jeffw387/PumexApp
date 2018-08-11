cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)
project(PumexApp LANGUAGES C CXX VERSION 0.0.1)

execute_process(COMMAND git submodule update --init -- external/pumex
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

include(ExternalProject)
                    
set(pumex_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/pumex")
ExternalProject_Add(pumex )