#
# Copyright (C) 2022 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.19)

project (LumeMetaAPITest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependencies
find_package(LumeMetaAPI REQUIRED)
find_package(LumeMetaExtAPI REQUIRED)
find_package(LumeMetaHighLevelAPI REQUIRED)
find_package(LumeMeta REQUIRED)
find_package(AGPEngineDLL REQUIRED)

if(NOT TARGET gtest)
    add_subdirectory(${BASE_ROOT_DIRECTORY}/test/3rdparty/googletest test/3rdparty/googletest EXCLUDE_FROM_ALL)
endif()

# Platform interface
set(common_test_sources
    ../common/log_util.cpp
    ../common/log_util.h
    ../common/test_runner_common.h
)
if(ANDROID)
list(APPEND common_test_sources
    ../common/test_runner_android.cpp
    ../common/test_runner_android.h
)
elseif(OHOS)
list(APPEND common_test_sources
    ../common/test_runner_ohos.cpp
    ../common/test_runner_ohos.h
)
elseif(WIN32)
list(APPEND common_test_sources
    ../common/test_runner_windows.cpp
    ../common/test_runner_windows.h
)
else() #Unix
list(APPEND common_test_sources
    ../common/test_runner_linux.cpp
    ../common/test_runner_linux.h
)
endif()

# For Clang format
set(all_common_test_sources
    ../common/log_util.cpp
    ../common/log_util.h
    ../common/test_runner_android.cpp
    ../common/test_runner_android.h
    ../common/test_runner_common.h
    ../common/test_runner_linux.cpp
    ../common/test_runner_linux.h
    ../common/test_runner_ohos.cpp
    ../common/test_runner_ohos.h
    ../common/test_runner_windows.cpp
    ../common/test_runner_windows.h
)

# using hash(#) marks for additional information
## testing category
#  related info about test/s
set(test_sources
    src/test_runner.cpp
    src/test_runner.h

    # Helpers
    src/helpers/animation_test_base.cpp
    src/helpers/animation_test_base.h
    src/helpers/serialisation_utils.cpp
    src/helpers/serialisation_utils.h
    src/helpers/test_data_helpers.h
    src/helpers/test_utils.cpp
    src/helpers/test_utils.h
    src/helpers/testing_macros.h
    src/helpers/testing_objects.cpp
    src/helpers/testing_objects.h
    src/helpers/util.h

    # API
    src/api/bitfield_property_test.cpp
    src/api/compatible_value_util_test.cpp
    src/api/deferred_callback_test.cpp
    src/api/function_test.cpp
    src/api/mutex_test.cpp
    src/api/number_test.cpp
    src/api/object_test.cpp
    src/api/property/array_property_event_handler_test.cpp
    src/api/property/custom_value_test.cpp
    src/api/property/property_info_test.cpp
    src/api/timer_test.cpp
    
    # Base
    src/base/base_objects_test.cpp
    src/base/capture_test.cpp
    src/base/ids_test.cpp
    src/base/ref_uri_test.cpp
    src/base/spin_lock_test.cpp
    src/base/time_span_test.cpp
    src/base/version_test.cpp
    
    # Ext
    src/ext/interface_helpers_test.cpp
    
    # Interface
    src/interface/animation/animation_controller_test.cpp
    src/interface/animation/animation_seeking_test.cpp
    src/interface/animation/bezier_curve_test.cpp
    src/interface/animation/intf_animation_modifier_test.cpp
    src/interface/animation/intf_animation_test.cpp
    src/interface/animation/intf_staggered_animation_test.cpp
    src/interface/animation/intf_track_animation_test.cpp
    src/interface/animation/sequential_animation_test.cpp
    src/interface/engine/engine_test_property.h
    src/interface/engine/intf_engine_input_property_manager_test.cpp
    src/interface/engine/intf_engine_value_manager_test.cpp
    src/interface/intf_any_test.cpp
    src/interface/intf_attach_test.cpp
    src/interface/intf_call_context_test.cpp
    src/interface/intf_class_registry_test.cpp
    src/interface/intf_container_observer_test.cpp
    src/interface/intf_container_query_test.cpp
    src/interface/intf_container_test.cpp
    src/interface/intf_content_test.cpp
    src/interface/intf_enum_test.cpp
    src/interface/intf_metadata_test.cpp
    src/interface/intf_object_hierarchy_observer_test.cpp
    src/interface/intf_object_registry_test.cpp
    src/interface/intf_proxy_object_test.cpp
    src/interface/intf_startable_controller_test.cpp
    src/interface/intf_task_queue_registry_test.cpp
    src/interface/intf_task_queue_test.cpp
    src/interface/loaders/csv_string_resource_loader_test.cpp
    src/interface/model/intf_data_model_test.cpp
    src/interface/model/intf_object_provider_test.cpp
    src/interface/property/array_property_test.cpp
    src/interface/property/intf_modifier_test.cpp
    src/interface/property/property_test.cpp
    src/interface/resource/intf_resource_manager_test.cpp
    src/interface/serialization/intf_serializable_test.cpp
    src/interface/serialization/property_serialization_test.cpp
    src/interface/serialization/resource_serialization_test.cpp
    src/interface/serialization/ser_compatibility_test.cpp
    src/interface/serialization/serialization_test.cpp
    src/interface/simple_event_test.cpp
)

set_source_files_properties(src/interface/serialization/serialization_test.cpp PROPERTIES COMPILE_FLAGS $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/bigobj>)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${test_sources})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/../ FILES ${common_test_sources})

#
# Test runner exe/library.
#
if(OHOS)
    set(executableName entry)
else()
    set(executableName LumeMetaAPITestRunner)
endif()

if(ANDROID)
    find_library(android_lib android)
    find_library(log_lib log)
    add_library(${executableName} SHARED ${common_test_sources} ${test_sources} "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c")
    target_include_directories(${executableName} PRIVATE "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")
    target_link_libraries(${executableName} ${android_lib} ${log_lib})
elseif(OHOS) 
    add_library(${executableName} SHARED ${common_test_sources} ${test_sources})
    target_link_libraries(${executableName} libace_napi.z.so libace_ndk.z.so libhilog_ndk.z.so libbundle_ndk.z.so librawfile.z.so)
else()
    add_executable(${executableName} ${common_test_sources} ${test_sources})
    target_compile_definitions(${executableName} PRIVATE WIN32_LEAN_AND_MEAN=1 NOMINMAX)
endif()

target_include_directories(${executableName} PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/../common
)

if(MSVC)
    target_compile_options(${executableName} PRIVATE /GR-)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(${executableName} PRIVATE -fno-rtti)
    if(META_UNIT_TESTS_WITH_SANITIZERS AND NOT ANDROID AND NOT OHOS)
        target_compile_options(${executableName} PRIVATE -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fsanitize-recover=address)
        target_link_options(${executableName} PRIVATE -fsanitize=address -fsanitize=undefined)
    endif()
endif()

# Sanitizers
if(META_UNIT_TESTS_WITH_SANITIZERS AND ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") AND NOT ANDROID AND NOT OHOS)
    target_compile_options(LumeMeta PRIVATE -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fsanitize-recover=address)
    target_link_options(LumeMeta PRIVATE -fsanitize=address -fsanitize=undefined)
endif()

# XCB server for Linux platform
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    #Include XCB for surface creation on X11
    find_package(X11 REQUIRED COMPONENTS xcb)
    include_directories(${X11_xcb_INCLUDE_PATH})
    target_link_libraries(${executableName} ${X11_xcb_LIB})
endif()

target_link_libraries(${executableName}
    AGPEngine::AGPEngineDLL # to avoid manual dll loading
    LumeMeta::ExtAPI
    gtest
    gmock
)

#
# Add plugin dependencies
#
add_dependencies(${executableName} LumeMeta::Plugin)

if(MINGW)
    # Linking mingw and c/c++ standard libs statically for easier usage.
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
endif()

# Cmake helper TargetPluginsDependencies doesn't copy pdb files
#include(TargetPluginsDependencies)
#
#target_plugins_dependencies(${executableName} 
#        PLUGINS AGPEngine::AGPEngineDLL
#        RELATIVE_DEPLOY_PATH .
#        VERBOSE)
#
#target_plugins_dependencies(${executableName} 
#        PLUGINS LumeMeta::Plugin
#        RELATIVE_DEPLOY_PATH "plugins"
#        VERBOSE)

##magicFile_ used to trigger the required copies..
#target_sources(LumeMetaTestRunner PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/magicFile_.txt)
#add_custom_command(
#    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/magicFile_.txt
#    COMMAND echo "Copying test assets"
#
#    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:LumeMetaTestRunner>/assets/
#    COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/assets/ $<TARGET_FILE_DIR:LumeMetaTestRunner>/assets/
#
#    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/magicFile.txt
#)

# Visual studio specific config. so the debugger working directory matches default QtCreator. and is WITH the executable.
#set_target_properties(LumeMetaTestRunner PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:LumeMetaTestRunner>")

if(NOT OHOS)
    #Required copies..
    add_custom_command(
        TARGET ${executableName} POST_BUILD

        # Plugins
	    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${executableName}>/plugins/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:LumeMeta::Plugin> $<TARGET_FILE_DIR:${executableName}>/plugins/

	    # Shared and common assets/resources
	    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${executableName}>/../../assets/
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../assets/ $<TARGET_FILE_DIR:${executableName}>/../../assets/
    )
else()
    add_custom_command(
        TARGET ${executableName} POST_BUILD

        # Plugins
	    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${executableName}>/plugins/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:LumeMeta::Plugin> $<TARGET_FILE_DIR:${executableName}>/plugins/
    
        # Shared and common assets/resources
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../assets/ $<TARGET_FILE_DIR:${executableName}>/../../../../res/default/resources/rawfile/
    )
endif()

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
# Add CTest
#
if(NOT ANDROID AND NOT OHOS)
    enable_testing()
    add_test(NAME ${PROJECT_NAME} COMMAND ${executableName})
endif()

#
# Clang format target
#
add_custom_target(Format${executableName}
    COMMAND clang-format -verbose -i ${all_common_test_sources} ${test_sources}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
set_target_properties(Format${executableName} PROPERTIES FOLDER Formatting)
