
#
# Copyright (C) 2022 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.19)

project (LumeMetaBenchmark)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LumeMetaAPI REQUIRED)
find_package(LumeMetaExtAPI REQUIRED)
find_package(LumeMetaHighLevelAPI REQUIRED)
find_package(LumeMeta QUIET)
find_package(AGPBaseAPI REQUIRED)
find_package(AGPEngine REQUIRED)

# Google benchmark
if(NOT TARGET benchmark)
    add_subdirectory(${LUMEBASE_PATH}/test/3rdparty/benchmark test/3rdparty/benchmark EXCLUDE_FROM_ALL)
endif()

#
# Benchmark app name.
#
set(executableName LumeMetaBenchmark)

set(benchmark_sources
    src/main.cpp
    src/metadata_benchmarks.cpp
    src/object_registry_benchmarks.cpp
    src/object_benchmarks.cpp
    src/property_benchmarks.cpp
    src/property_utils.h
    src/shared_ptr_benchmarks.cpp
    src/task_queue_benchmarks.cpp
    src/utils.cpp
    src/utils.h
)

add_executable(${executableName} ${benchmark_sources})

target_link_libraries(${executableName} PRIVATE
    AGPBase::AGPBaseAPI
    AGPEngine::AGPEngineAPI
    AGPEngine::AGPEngineDLL 
    LumeMeta::ExtAPI
    LumeMeta::HighLevelAPI
    benchmark
)

# Cmake helper TargetPluginsDependencies doesn't copy pdb files
#include(TargetPluginsDependencies)
#
#target_plugins_dependencies(LumeMetaBenchmark
#        PLUGINS AGPEngine::AGPEngineDLL
#        RELATIVE_DEPLOY_PATH .
#        VERBOSE)
#
#target_plugins_dependencies(LumeMetaBenchmark
#        PLUGINS LumeMeta::Plugin
#        RELATIVE_DEPLOY_PATH "plugins"
#        VERBOSE)

#Required copies..
add_custom_command(
    TARGET ${executableName} POST_BUILD

    # Plugins
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${executableName}>/plugins/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:LumeMeta::Plugin> $<TARGET_FILE_DIR:${executableName}>/plugins/
    
    # Shared and common assets/resources
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(COPY_TARGET_PDB
        ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PDB_FILE:AGPEngine::AGPEngineDLL>    $<TARGET_FILE_DIR:${executableName}> &&
        ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PDB_FILE:LumeMeta::Plugin>         $<TARGET_FILE_DIR:${executableName}>/plugins/ 
    )
    add_custom_command(
        TARGET ${executableName} POST_BUILD

        # DLLs
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AGPEngine::AGPEngineDLL> $<TARGET_FILE_DIR:${executableName}>

        # PDB files
        COMMAND "$<IF:$<CONFIG:Debug,RelWithDebInfo>,${COPY_TARGET_PDB},>"
        COMMAND_EXPAND_LISTS
    )
endif()

set_target_properties(${executableName} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${executableName}>")

set_target_properties(${executableName} PROPERTIES FOLDER Testing)

#
# Clang format target
#
add_custom_target(Format${executableName}
    COMMAND clang-format -verbose -i ${benchmark_sources}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
set_target_properties(Format${executableName} PROPERTIES FOLDER Formatting)
