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
set(ENGINE_COMMON_DIR ${CORE_ROOT_DIRECTORY}/test/unittest/common)
set(BASE_COMMON_DIR ${BASE_ROOT_DIRECTORY}/test/unittest/common)
include(${BASE_ROOT_DIRECTORY}/test/unittest/cmake/AgpTestCommon.cmake)
agp_collect_test_common_sources(
    ENGINE_DIR ${ENGINE_COMMON_DIR}
    BASE_DIR ${BASE_COMMON_DIR}
    COMPILE_LOAD_LIB
    LOCAL_DIR ${CMAKE_CURRENT_LIST_DIR}/../common
)

if(ANDROID)
    list(APPEND common_test_sources src/test_runner_android_entry.cpp)
endif()

# For Clang format
# all_common_test_sources comes from agp_collect_test_common_sources above.

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
    src/interface/alias/alias_engine_binding_test.cpp
    src/interface/alias/alias_registry_test.cpp
    src/interface/animation/animation_controller_test.cpp
    src/interface/animation/animation_seeking_test.cpp
    src/interface/animation/bezier_curve_test.cpp
    src/interface/animation/intf_animation_modifier_test.cpp
    src/interface/animation/intf_animation_test.cpp
    src/interface/animation/intf_staggered_animation_test.cpp
    src/interface/animation/intf_track_animation_test.cpp
    src/interface/animation/sequential_animation_test.cpp
    src/interface/animation/interpolator_test.cpp
    src/interface/animation/easing_curve_test.cpp
    src/interface/engine/engine_test_property.h
    src/interface/engine/intf_engine_value_manager_test.cpp
    src/interface/engine/engine_value_test.cpp
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
    src/interface/property/array_property_test.cpp
    src/interface/property/intf_modifier_test.cpp
    src/interface/property/property_test.cpp
    src/interface/resource/intf_resource_manager_test.cpp
    src/interface/serialization/debug_output_test.cpp
    src/interface/serialization/intf_serializable_test.cpp
    src/interface/serialization/property_serialization_test.cpp
    src/interface/serialization/resource_serialization_test.cpp
    src/interface/serialization/ser_compatibility_test.cpp
    src/interface/serialization/serialization_test.cpp
    src/interface/simple_event_test.cpp
)

set_source_files_properties(src/interface/serialization/serialization_test.cpp PROPERTIES COMPILE_FLAGS $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/bigobj>)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${test_sources})
# common sources are grouped by agp_collect_test_common_sources.

#
# Test runner exe/library.
#
set(executableName LumeMetaAPITestRunner)
if(ANDROID)
    find_library(android_lib android)
    find_library(log_lib log)
    add_library(${executableName} SHARED ${common_test_sources} ${test_sources} "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c")
    target_include_directories(${executableName} PRIVATE "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")
    target_link_libraries(${executableName} ${android_lib} ${log_lib})
elseif(OHOS)
    add_executable(${executableName} ${common_test_sources} ${test_sources})
    target_link_libraries(${executableName} libhilog_ndk.z.so)
else()
    add_executable(${executableName} ${common_test_sources} ${test_sources})
    target_compile_definitions(${executableName} PRIVATE WIN32_LEAN_AND_MEAN=1 NOMINMAX)
endif()

target_compile_definitions(${executableName} PRIVATE CORE_DYNAMIC=1 TEST_RUNNER_NAME="MetaTestRunner" TEST_LOG_TAG="meta_unit_test")

target_include_directories(${executableName} PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/../common
    ${ENGINE_COMMON_DIR}
    ${BASE_COMMON_DIR}
)

if(MSVC)
    target_compile_options(${executableName} PRIVATE /GR- /bigobj)
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
    LumeMeta::ExtAPI
    LumeMeta::Plugin
    gtest
    gmock
)

if(MINGW)
    # Linking mingw and c/c++ standard libs statically for easier usage.
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
endif()

# Required copies -- uses a custom target so rebuilt plugins are always copied,
# even when the test runner itself does not need re-linking.
add_custom_target(${executableName}_CopyDeps ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${executableName}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AGPEngine::AGPEngineDLL> $<TARGET_FILE_DIR:${executableName}>
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${executableName}>/plugins/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:LumeMeta::Plugin> $<TARGET_FILE_DIR:${executableName}>/plugins/
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${executableName}>/../../assets/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../assets/ $<TARGET_FILE_DIR:${executableName}>/../../assets/
    COMMENT "Copying dependencies for ${executableName}"
)
add_dependencies(${executableName}_CopyDeps AGPEngine::AGPEngineDLL LumeMeta::Plugin)
add_dependencies(${executableName} ${executableName}_CopyDeps)
set_target_properties(${executableName}_CopyDeps PROPERTIES FOLDER Testing)

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
