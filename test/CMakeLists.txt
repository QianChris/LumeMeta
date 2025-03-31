#
# Copyright (C) 2021-2023 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.19)

project (MetaObjectTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MetaObjectAPI REQUIRED)
find_package(MetaObjectExtAPI REQUIRED)
find_package(MetaObjectHighLevelAPI REQUIRED)
find_package(MetaObject QUIET)

# using hash(#) marks for additional information
## testing category
#  related info about test/s
set(test_sources
    src/animation_test_base.cpp
    src/animation_test_base.h
    src/api/property/test_array_property_event_handler.cpp
    src/api/property/test_custom_value.cpp
    src/api/property/test_property_info.cpp
    src/api/test_bitfield_property.cpp
    src/api/test_compatible_value_util.cpp
    src/api/test_deferred_callback.cpp
    src/api/test_function.cpp
    src/api/test_mutex.cpp
    src/api/test_number.cpp
    src/api/test_object.cpp
    src/api/test_timer.cpp
    src/base/test_base_objects.cpp
    src/base/test_capture.cpp
    src/base/test_ids.cpp
    src/base/test_ref_uri.cpp
    src/base/test_spin_lock.cpp
    src/base/test_time_span.cpp
    src/base/test_version.cpp
    src/ext/test_interface_helpers.cpp
    src/interface/animation/test_animation_controller.cpp
    src/interface/animation/test_animation_seeking.cpp
    src/interface/animation/test_bezier_curve.cpp
    src/interface/animation/test_intf_animation.cpp
    src/interface/animation/test_intf_animation_modifier.cpp
    src/interface/animation/test_intf_staggered_animation.cpp
    src/interface/animation/test_intf_track_animation.cpp
    src/interface/animation/test_sequential_animation.cpp
    src/interface/engine/engine_test_property.h
    src/interface/engine/test_intf_engine_input_property_manager.cpp
    src/interface/engine/test_intf_engine_value_manager.cpp
    src/interface/loaders/test_csv_string_resource_loader.cpp
    src/interface/model/test_intf_data_model.cpp
    src/interface/model/test_intf_object_provider.cpp
    src/interface/property/test_array_property.cpp
    src/interface/property/test_intf_modifier.cpp
    src/interface/property/test_property.cpp
    src/interface/resource/test_file_resource_manager.cpp
    src/interface/serialization/test_intf_serializable.cpp
    src/interface/serialization/test_property_serialization.cpp
    src/interface/serialization/test_resource_serialization.cpp
    src/interface/serialization/test_ser_compatibility.cpp
    src/interface/serialization/test_serialization.cpp
    src/interface/test_intf_any.cpp
    src/interface/test_intf_attach.cpp
    src/interface/test_intf_call_context.cpp
    src/interface/test_intf_class_registry.cpp
    src/interface/test_intf_container.cpp
    src/interface/test_intf_container_observer.cpp
    src/interface/test_intf_container_query.cpp
    src/interface/test_intf_content.cpp
    src/interface/test_intf_enum.cpp
    src/interface/test_intf_metadata.cpp
    src/interface/test_intf_object_hierarchy_observer.cpp
    src/interface/test_intf_object_registry.cpp
    src/interface/test_intf_proxy_object.cpp
    src/interface/test_intf_startable_controller.cpp
    src/interface/test_intf_task_queue.cpp
    src/interface/test_intf_task_queue_registry.cpp
    src/interface/test_simple_event.cpp
    src/serialisation_utils.cpp
    src/serialisation_utils.h
    src/test_data_helpers.h
    src/test_macros.h
    src/test_utils.cpp
    src/test_utils.h
    src/tester.cpp
    src/tester.h
    src/testing_objects.cpp
    src/testing_objects.h
    src/util.h
    )

set_source_files_properties(src/interface/serialization/test_serialization.cpp PROPERTIES COMPILE_FLAGS $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/bigobj>)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${test_sources})

add_executable(MetaObjectTestRunner ${test_sources})
target_compile_definitions(MetaObjectTestRunner PRIVATE WIN32_LEAN_AND_MEAN=1 CORE_DYNAMIC=1)
target_include_directories(MetaObjectTestRunner PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

target_link_libraries(MetaObjectTestRunner MetaObject::ExtAPI gmock MetaObject::EnvironmentSetup)

if(MINGW)
    # Linking mingw and c/c++ standard libs statically for easier usage.
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
endif()

include(TargetPluginsDependencies)

target_plugins_dependencies(MetaObjectTestRunner
        PLUGINS AGPEngine::AGPEngineDLL
        RELATIVE_DEPLOY_PATH .
        VERBOSE)

target_plugins_dependencies(MetaObjectTestRunner
        PLUGINS MetaObject::Plugin
        RELATIVE_DEPLOY_PATH "plugins"
        VERBOSE)

#magicFile_ used to trigger the required copies..
target_sources(MetaObjectTestRunner PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/magicFile_.txt)
add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/magicFile_.txt
    COMMAND echo "Copying test assets"

    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:MetaObjectTestRunner>/assets/
    COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/assets/ $<TARGET_FILE_DIR:MetaObjectTestRunner>/assets/

    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/magicFile.txt
)

# Visual studio specific config. so the debugger working directory matches default QtCreator. and is WITH the executable.
set_target_properties(MetaObjectTestRunner PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:MetaObjectTestRunner>")

#
# Clang format target
#
add_custom_target(FormatMetaObjectTestRunner
    COMMAND clang-format -verbose -i ${test_sources}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
set_target_properties(FormatMetaObjectTestRunner PROPERTIES FOLDER Formatting)
