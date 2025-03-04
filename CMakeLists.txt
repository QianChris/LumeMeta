cmake_minimum_required(VERSION 3.19)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(META_VISUAL_STUDIO_TIDY  "Create hackish targets to run clang-tidy steps in visual studio" OFF)
option(META_SUBMODULE_ENABLED "USe MetaObject as submodule" OFF)
if(DEFINED ENV{LUME_PACKAGE_CACHE})
    set(PACKAGE_CACHE_PATH $ENV{LUME_PACKAGE_CACHE} CACHE PATH "Path to lume package cache")
else()
    set(PACKAGE_CACHE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/packagecache CACHE PATH "Path to lume package cache")
endif()
if (EXISTS ${PACKAGE_CACHE_PATH})
    set(HAS_NO_PACKAGE_CACHE false)
    set(HAS_PACKAGE_CACHE true)
else()
    set(HAS_NO_PACKAGE_CACHE true)
    set(HAS_PACKAGE_CACHE false)
endif()
option(BUILD_DEPS "Try to build deps from submodules" ${HAS_NO_PACKAGE_CACHE})

set(LUMEBASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/submodules/LumeBase CACHE PATH "Path to lume base sources")
set(LUMEENGINE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/submodules/LumeEngine CACHE PATH "Path to lume engine sources")
set(CORE_UNPACKED_LIB_DIR ${CMAKE_BINARY_DIR}/unpacked CACHE PATH "Path where to extract 3rd party library sources.")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
if (${BUILD_DEPS})
    if (EXISTS ${LUMEBASE_PATH}/CMakeLists.txt AND
        EXISTS ${LUMEENGINE_PATH}/CMakeLists.txt)
        list(APPEND CMAKE_MODULE_PATH "${LUMEBASE_PATH}/cmake")
        list(APPEND CMAKE_MODULE_PATH "${LUMEENGINE_PATH}/cmake")
    else()
        message(FATAL_ERROR "Requested to build Lume Core library, but sources not found!")
    endif()
else()
    message("Using LumeBase and LumeEngine from package cache.")
    list(APPEND CMAKE_MODULE_PATH "${PACKAGE_CACHE_PATH}/LumeBase/cmake")
    list(APPEND CMAKE_MODULE_PATH "${PACKAGE_CACHE_PATH}/LumeEngine/cmake")
endif()

project (MetaObject LANGUAGES CXX)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # enable multi-processor compilation.
    add_compile_options(/MP)
    # also make sure the external/system include path definition extension is enabled! (if available)
    if (MSVC_VERSION GREATER_EQUAL 1913) # 1913+ has the extension..
        if (MSVC_VERSION LESS 1929) # pre 1929 requires enabling the extensions as it was still experimental.
            add_compile_options(/experimental:external)
        endif()

        # disable all warnings for "system/3rdparty" headers.
        add_compile_options(/external:W0)

        if (${CMAKE_VERSION} VERSION_LESS "3.24.0")
            #CMAKE pre 3.24 does not support the CMAKE_INCLUDE_SYSTEM_FLAG_CXX option.
            #so declare that all #include <> are "system", to atleast partially handle them.
            add_compile_options(/external:anglebrackets)
        else()
            #CMAKE post 3.24+ supports the CMAKE_INCLUDE_SYSTEM_FLAG_CXX option.
            # for reasons.. clang-tidy seems to have issues with /external:I, so use -external:I
            set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-external:I" CACHE INTERNAL "")
        endif()
    endif()
    if(MSVC_VERSION GREATER_EQUAL 1930)
        add_compile_options(/permissive-)
    endif()
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(WIN32)
        add_compile_options(-Wall -Wreturn-type -Werror=return-type -fdiagnostics-color=auto -Wno-deprecated-declarations -Wno-unused -Wno-overloaded-virtual -Wno-unqualified-std-cast-call)
    endif()
endif()

include(MetaObjectTargetCodeInspection)


if (BUILD_DEPS AND NOT META_SUBMODULE_ENABLED)
    # Find_package macro will workaround the findpackage issues (ie. if the target exists, then do nothing)
    # do this and that, since well.. typos.
    # (core libs have their cmake paths, but there is no configs/modules for source builds for all modules..)
    macro(find_package)
        if (NOT TARGET ${ARGV0})
            #message("find_package from ${CMAKE_CURRENT_LIST_FILE} ${ARGV} (does not exist yet)")
            _find_package(${ARGV})
        endif()
    endmacro()
    set(CMAKE_FOLDER "CoreLibraries")
    message("Found LumeBase Source, Building ${LUMEBASE_PATH}")
    set(BASE_ROOT_DIRECTORY ${LUMEBASE_PATH} CACHE PATH "" FORCE)
    add_subdirectory(${LUMEBASE_PATH} 3rdParty/LumeBase EXCLUDE_FROM_ALL)

    message("Found LumeEngine Source, Building ${LUMEENGINE_PATH}")
    set(CORE_BUILD_BASE OFF)
    set(CORE_ROOT_DIRECTORY ${LUMEENGINE_PATH} CACHE PATH "" FORCE)
    add_subdirectory(${LUMEENGINE_PATH}/api/ 3rdParty/LumeEngine/api EXCLUDE_FROM_ALL)
    add_subdirectory(${LUMEENGINE_PATH} 3rdParty/LumeEngine EXCLUDE_FROM_ALL)
    set_target_properties(AGPEngineDLL PROPERTIES FOLDER CoreLibraries/DLLs)
    unset(CMAKE_FOLDER)
endif()

find_package(AGPEngineAPI REQUIRED)
find_package(AGPEngineDLL QUIET)

add_library(MetaObjectBase INTERFACE)
target_sources(MetaObjectBase PUBLIC
    include/meta/base/algorithms.h
    include/meta/base/bit_field.h
    include/meta/base/capture.h
    include/meta/base/expected.h
    include/meta/base/ids.h
    include/meta/base/interface_macros.h
    include/meta/base/interface_traits.h
    include/meta/base/interface_utils.h
    include/meta/base/macros.h
    include/meta/base/meta_types.h
    include/meta/base/namespace.h
    include/meta/base/object_traits.h
    include/meta/base/plugin.h
    include/meta/base/ref_uri.h
    include/meta/base/time_span.h
    include/meta/base/type_traits.h
    include/meta/base/types.h
    include/meta/base/version.h
    )

target_include_directories(MetaObjectBase INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include/>)
target_link_libraries(MetaObjectBase INTERFACE AGPBase::AGPBaseAPI AGPEngine::AGPEngineAPI)

# meta object interface (for users of meta objects)
add_library(MetaObjectAPI INTERFACE)
add_library(MetaObject::API ALIAS MetaObjectAPI)
target_sources(MetaObjectAPI PUBLIC
    include/meta/interface/animation/builtin_animations.h
    include/meta/interface/animation/intf_animation.h
    include/meta/interface/animation/intf_animation_controller.h
    include/meta/interface/animation/intf_animation_modifier.h
    include/meta/interface/animation/intf_interpolator.h
    include/meta/interface/animation/modifiers/intf_loop.h
    include/meta/interface/animation/modifiers/intf_speed.h
    include/meta/interface/builtin_objects.h
    include/meta/interface/curves/intf_bezier.h
    include/meta/interface/curves/intf_curve.h
    include/meta/interface/curves/intf_curve_1d.h
    include/meta/interface/curves/intf_curve_2d.h
    include/meta/interface/curves/intf_easing_curve.h
    include/meta/interface/detail/any.h
    include/meta/interface/detail/any_pointer_compatibility.h
    include/meta/interface/detail/array_property.h
    include/meta/interface/detail/enum.h
    include/meta/interface/detail/multi_type_any.h
    include/meta/interface/detail/property.h
    include/meta/interface/engine/intf_engine_data.h
    include/meta/interface/engine/intf_engine_input_property_manager.h
    include/meta/interface/engine/intf_engine_type.h
    include/meta/interface/engine/intf_engine_value.h
    include/meta/interface/engine/intf_engine_value_manager.h
    include/meta/interface/enum_macros.h
    include/meta/interface/event.h
    include/meta/interface/interface_helpers.h
    include/meta/interface/interface_macros.h
    include/meta/interface/intf_any.h
    include/meta/interface/intf_attach.h
    include/meta/interface/intf_attachment.h
    include/meta/interface/intf_attachment_container.h
    include/meta/interface/intf_call_context.h
    include/meta/interface/intf_callable.h
    include/meta/interface/intf_class_registry.h
    include/meta/interface/intf_clock.h
    include/meta/interface/intf_cloneable.h
    include/meta/interface/intf_comparable.h
    include/meta/interface/intf_connector.h
    include/meta/interface/intf_containable.h
    include/meta/interface/intf_container.h
    include/meta/interface/intf_container_observer.h
    include/meta/interface/intf_container_query.h
    include/meta/interface/intf_content.h
    include/meta/interface/intf_derived.h
    include/meta/interface/intf_enum.h
    include/meta/interface/intf_event.h
    include/meta/interface/intf_function.h
    include/meta/interface/intf_future.h
    include/meta/interface/intf_iterable.h
    include/meta/interface/intf_lifecycle.h
    include/meta/interface/intf_lockable.h
    include/meta/interface/intf_manual_clock.h
    include/meta/interface/intf_meta_object_lib.h
    include/meta/interface/intf_metadata.h
    include/meta/interface/intf_named.h
    include/meta/interface/intf_notify_on_change.h
    include/meta/interface/intf_object.h
    include/meta/interface/intf_object_context.h
    include/meta/interface/intf_object_factory.h
    include/meta/interface/intf_object_flags.h
    include/meta/interface/intf_object_hierarchy_observer.h
    include/meta/interface/intf_object_registry.h
    include/meta/interface/intf_promise.h
    include/meta/interface/intf_proxy_object.h
    include/meta/interface/intf_recyclable.h
    include/meta/interface/intf_required_interfaces.h
    include/meta/interface/intf_startable.h
    include/meta/interface/intf_startable_controller.h
    include/meta/interface/intf_static_metadata.h
    include/meta/interface/intf_task_queue.h
    include/meta/interface/intf_task_queue_extend.h
    include/meta/interface/intf_task_queue_registry.h
    include/meta/interface/intf_tickable.h
    include/meta/interface/intf_tickable_controller.h
    include/meta/interface/intf_value.h
    include/meta/interface/loaders/intf_class_content_loader.h
    include/meta/interface/loaders/intf_content_loader.h
    include/meta/interface/loaders/intf_dynamic_content_loader.h
    include/meta/interface/loaders/intf_file_content_loader.h
    include/meta/interface/metadata_query.h
    include/meta/interface/model/data_model_index.h
    include/meta/interface/model/intf_data_model.h
    include/meta/interface/model/intf_object_provider.h
    include/meta/interface/object_macros.h
    include/meta/interface/object_type_info.h
    include/meta/interface/property/array_property.h
    include/meta/interface/property/construct_array_property.h
    include/meta/interface/property/construct_property.h
    include/meta/interface/property/intf_bind.h
    include/meta/interface/property/intf_modifier.h
    include/meta/interface/property/intf_property.h
    include/meta/interface/property/intf_property_internal.h
    include/meta/interface/property/intf_property_register.h
    include/meta/interface/property/intf_stack_property.h
    include/meta/interface/property/intf_stack_resetable.h
    include/meta/interface/property/property.h
    include/meta/interface/property/property_events.h
    include/meta/interface/resource/intf_dynamic_resource.h
    include/meta/interface/resource/intf_resource.h
    include/meta/interface/resource/intf_resource_container.h
    include/meta/interface/serialization/intf_export_context.h
    include/meta/interface/serialization/intf_exporter.h
    include/meta/interface/serialization/intf_global_serialization_data.h
    include/meta/interface/serialization/intf_import_context.h
    include/meta/interface/serialization/intf_importer.h
    include/meta/interface/serialization/intf_ser_input.h
    include/meta/interface/serialization/intf_ser_node.h
    include/meta/interface/serialization/intf_ser_output.h
    include/meta/interface/serialization/intf_ser_transformation.h
    include/meta/interface/serialization/intf_serializable.h
    include/meta/interface/serialization/intf_value_serializer.h
    include/meta/interface/simple_event.h
    include/meta/interface/static_object_metadata.h
    include/meta/interface/threading/primitive_api.h
    )

target_include_directories(MetaObjectAPI INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include/>)
target_link_libraries(MetaObjectAPI INTERFACE MetaObjectBase)

# Meta object helper implementations. (for implementers of meta objects)
add_library(MetaObjectHighLevelAPI INTERFACE)
add_library(MetaObject::HighLevelAPI ALIAS MetaObjectHighLevelAPI)
target_sources(MetaObjectHighLevelAPI PUBLIC
    include/meta/api/animation/animation.h
    include/meta/api/animation/modifiers/loop.h
    include/meta/api/animation/modifiers/reverse.h
    include/meta/api/animation/modifiers/speed.h
    include/meta/api/array_util.h
    include/meta/api/call_context.h
    include/meta/api/compatible_value_util.h
    include/meta/api/connector.h
    include/meta/api/container/container_observer.h
    include/meta/api/container/find_cache.h
    include/meta/api/container/find_containers.h
    include/meta/api/container/object_hierarchy_observer.h
    include/meta/api/content_object.h
    include/meta/api/curves/cubic_bezier_easing_curve.h
    include/meta/api/curves/easing_curve.h
    include/meta/api/deferred_callback.h
    include/meta/api/engine/util.h
    include/meta/api/event_handler.h
    include/meta/api/function.h
    include/meta/api/future.h
    include/meta/api/internal/animation_api.h
    include/meta/api/internal/animation_modifier_api.h
    include/meta/api/internal/breadth_first_iteration.h
    include/meta/api/internal/depth_first_iteration.h
    include/meta/api/internal/iteration.h
    include/meta/api/internal/iteration_types.h
    include/meta/api/internal/object_api.h
    include/meta/api/iteration.h
    include/meta/api/loaders/json_content_loader.h
    include/meta/api/locking.h
    include/meta/api/make_callback.h
    include/meta/api/model/container_data_model.h
    include/meta/api/model/content_loader_object_provider.h
    include/meta/api/model/instantiating_object_provider.h
    include/meta/api/number.h
    include/meta/api/object.h
    include/meta/api/property/array_element_bind.h
    include/meta/api/property/array_property_changes_recognizer.h
    include/meta/api/property/array_property_event_handler.h
    include/meta/api/property/binding.h
    include/meta/api/property/custom_value.h
    include/meta/api/property/default_value_bind.h
    include/meta/api/property/property_event_handler.h
    include/meta/api/property/property_info.h
    include/meta/api/task.h
    include/meta/api/task_queue.h
    include/meta/api/threading/mutex.h
    include/meta/api/threading/primitive_api.h
    include/meta/api/timer.h
    include/meta/api/util.h
    )

target_include_directories(MetaObjectHighLevelAPI INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include/>)
target_link_libraries(MetaObjectHighLevelAPI INTERFACE MetaObjectAPI)

# Meta object helper implementations. (for implementers of meta objects)
add_library(MetaObjectExtAPI INTERFACE)
add_library(MetaObject::ExtAPI ALIAS MetaObjectExtAPI)
target_sources(MetaObjectExtAPI PUBLIC
    include/meta/ext/animation/interpolator.h
    include/meta/ext/any_builder.h
    include/meta/ext/attachment/attachment.h
    include/meta/ext/base_object.h
    include/meta/ext/base_object_fwd.h
    include/meta/ext/engine/core_any.h
    include/meta/ext/engine/core_enum_any.h
    include/meta/ext/engine/internal_access.h
    include/meta/ext/event_impl.h
    include/meta/ext/event_util.h
    include/meta/ext/implementation_macros.h
    include/meta/ext/metadata_helpers.h
    include/meta/ext/minimal_object.h
    include/meta/ext/object.h
    include/meta/ext/object_container.h
    include/meta/ext/object_factory.h
    include/meta/ext/object_fwd.h
    include/meta/ext/resolve_helper.h
    include/meta/ext/serialization/common_value_serializers.h
    include/meta/ext/serialization/serializer.h
    include/meta/ext/serialization/value_serializer.h
    include/meta/ext/task_queue.h
    )

target_include_directories(MetaObjectExtAPI INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include/>)
target_link_libraries(MetaObjectExtAPI INTERFACE MetaObjectAPI MetaObjectHighLevelAPI)

# Meta object plugin
add_library(MetaObject SHARED)
add_library(MetaObject::Plugin ALIAS MetaObject)
set_property(TARGET MetaObject PROPERTY CXX_STANDARD 17)
set_property(TARGET MetaObject PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET MetaObject PROPERTY CXX_EXTENSIONS off)
target_sources(MetaObject PRIVATE
    src/animation/animation.h
    src/animation/animation_controller.cpp
    src/animation/animation_controller.h
    src/animation/animation_modifier.h
    src/animation/animation_state.cpp
    src/animation/animation_state.h
    src/animation/interpolator.cpp
    src/animation/interpolator.h
    src/animation/intf_animation_internal.h
    src/animation/keyframe_animation.cpp
    src/animation/keyframe_animation.h
    src/animation/modifiers/loop.cpp
    src/animation/modifiers/loop.h
    src/animation/modifiers/reverse.cpp
    src/animation/modifiers/reverse.h
    src/animation/modifiers/speed.cpp
    src/animation/modifiers/speed.h
    src/animation/parallel_animation.cpp
    src/animation/parallel_animation.h
    src/animation/property_animation.cpp
    src/animation/property_animation.h
    src/animation/sequential_animation.cpp
    src/animation/sequential_animation.h
    src/animation/staggered_animation_state.cpp
    src/animation/staggered_animation_state.h
    src/animation/track_animation.cpp
    src/animation/track_animation.h
    src/animation/track_animation_state.cpp
    src/animation/track_animation_state.h
    src/attachment_container.cpp
    src/attachment_container.h
    src/base_object.h
    src/call_context.cpp
    src/call_context.h
    src/class_registry.cpp
    src/class_registry.h
    src/connector.cpp
    src/connector.h
    src/container/container.cpp
    src/container/container.h
    src/container/container_base.cpp
    src/container/container_base.h
    src/container/flat_container.cpp
    src/container/flat_container.h
    src/container/object_container.cpp
    src/container/object_container.h
    src/container/object_flat_container.cpp
    src/container/object_flat_container.h
    src/container_observer.cpp
    src/container_observer.h
    src/content_object.cpp
    src/curves/bezier_curve.cpp
    src/curves/bezier_curve.h
    src/curves/easing_curve.cpp
    src/curves/easing_curve.h
    src/engine/engine_input_property_manager.cpp
    src/engine/engine_input_property_manager.h
    src/engine/engine_value.cpp
    src/engine/engine_value.h
    src/engine/engine_value_manager.cpp
    src/engine/engine_value_manager.h
    src/functions.cpp
    src/functions.h
    src/future.cpp
    src/future.h
    src/loaders/class_content_loader.cpp
    src/loaders/class_content_loader.h
    src/loaders/csv_parser.cpp
    src/loaders/csv_parser.h
    src/loaders/csv_string_resource_loader.cpp
    src/loaders/csv_string_resource_loader.h
    src/loaders/json_content_loader.cpp
    src/loaders/json_content_loader.h
    src/manual_clock.cpp
    src/meta_object_lib.cpp
    src/meta_object_lib.h
    src/model/composite_object_provider.cpp
    src/model/composite_object_provider.h
    src/model/container_data_model.cpp
    src/model/container_data_model.h
    src/model/content_loader_object_provider.cpp
    src/model/content_loader_object_provider.h
    src/model/instantiating_object_provider.cpp
    src/model/instantiating_object_provider.h
    src/model/object_provider_base.cpp
    src/model/object_provider_base.h
    src/number.cpp
    src/number.h
    src/object.h
    src/object_context.cpp
    src/object_context.h
    src/object_data_container.cpp
    src/object_data_container.h
    src/object_hierarchy_observer.cpp
    src/object_hierarchy_observer.h
    src/object_name.h
    src/object_registry.cpp
    src/object_registry.h
    src/plugin.cpp
    src/polling_task_queue.cpp
    src/property/bind.cpp
    src/property/bind.h
    src/property/dependencies.cpp
    src/property/dependencies.h
    src/property/property.cpp
    src/property/property.h
    src/property/stack_property.cpp
    src/property/stack_property.h
    src/proxy_object.cpp
    src/proxy_object.h
    src/random.cpp
    src/random.h
    src/ref_uri_util.cpp
    src/ref_uri_util.h
    src/register_anys.cpp
    src/register_default_anims.cpp
    src/register_default_objs.cpp
    src/register_engine_access.cpp
    src/register_entities.cpp
    src/register_value_serializers.cpp
    src/resource/resource_container.h
    src/serialization/backend/debug_output.cpp
    src/serialization/backend/debug_output.h
    src/serialization/backend/json_input.cpp
    src/serialization/backend/json_input.h
    src/serialization/backend/json_output.cpp
    src/serialization/backend/json_output.h
    src/serialization/exporter.cpp
    src/serialization/exporter.h
    src/serialization/importer.cpp
    src/serialization/importer.h
    src/serialization/json_exporter.cpp
    src/serialization/json_exporter.h
    src/serialization/json_importer.cpp
    src/serialization/json_importer.h
    src/serialization/metav1_compat.cpp
    src/serialization/metav1_compat.h
    src/startable_object_controller.cpp
    src/startable_object_controller.h
    src/system_clock.cpp
    src/task_queue.h
    src/threaded_task_queue.cpp
    )

set_source_files_properties(src/register_default_props.cpp PROPERTIES COMPILE_FLAGS $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/bigobj>)

target_compile_definitions(MetaObject PRIVATE CORE_PLUGIN=1 INTERFACE CORE_DYNAMIC=1)

set_target_properties(MetaObject PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
    OUTPUT_NAME PluginMetaObject
    )

target_link_libraries(MetaObject PRIVATE MetaObjectAPI MetaObjectExtAPI MetaObjectHighLevelAPI)


get_target_property(IMPL_SOURCES MetaObject SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX src FILES ${IMPL_SOURCES})

get_target_property(BASE_SOURCES MetaObjectBase INTERFACE_SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/meta/base PREFIX base FILES ${BASE_SOURCES})

get_target_property(API_SOURCES MetaObjectAPI INTERFACE_SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/meta/interface PREFIX interface FILES ${API_SOURCES})

get_target_property(HIGHLEVEL_API_SOURCES MetaObjectHighLevelAPI INTERFACE_SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/meta/api PREFIX api FILES ${HIGHLEVEL_API_SOURCES})

get_target_property(EXT_API_SOURCES MetaObjectExtAPI INTERFACE_SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/meta/ext PREFIX ext FILES ${EXT_API_SOURCES})

meta_object_target_code_inspection(MetaObject)

option(META_TESTS_ENABLED "Build MetaObject unit tests" ON)

#
# Build tests if required.
#
if(META_TESTS_ENABLED)
    if (NOT TARGET gtest)
        # Add gtest if not already added, note that if there is a gtest version conflict your results may vary.
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/googletest 3rdparty/googletest EXCLUDE_FROM_ALL)
    endif()
    if (NOT TARGET benchmark)
        # Add benchmark if not already added, note that if there is a benchmark version conflict your results may vary.
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/benchmark 3rdparty/benchmark EXCLUDE_FROM_ALL)
    endif()
    add_subdirectory(environment_setup)
    add_subdirectory(test)
    add_subdirectory(benchmark)
    add_test(NAME metaobject COMMAND MetaObjectTestRunner --gtest_filter=*)
    enable_testing()
endif()

if (META_VISUAL_STUDIO_TIDY AND NOT META_SUBMODULE_ENABLED)
    # Somewhat hackish way to run the clang-tidy step from visual studio...
    message("Creating VS ExecTidyMeta in MetaObject cmakelists.txt")
    add_custom_target(ExecTidyMeta DEPENDS thatMetaTidy.txt
        OUTPUT  thatMetaTidy.txt
        COMMAND ${CMAKE_COMMAND} -DCORE_UNPACKED_LIB_DIR=${CORE_UNPACKED_LIB_DIR} -DPACKAGE_CACHE_PATH=${PACKAGE_CACHE_PATH} -DLUMEBASE_PATH=${LUMEBASE_PATH} -DLUMEENGINE_PATH=${LUMEENGINE_PATH} -DMETA_TESTS_ENABLED=${META_TESTS_ENABLED} -DBUILD_DEPS=${BUILD_DEPS} -DBUILD_LUME_CORE_LIBS=${BUILD_LUME_CORE_LIBS} -DLUME_META_DISABLE_RUNTIME_DEPENDENCIES=true -DLUME_META_OBJECT_PERFORM_CODE_INSPECTION=true -G "Ninja" -DCMAKE_BUILD_TYPE="Release" -S ${CMAKE_CURRENT_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/TidyMetaTask
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/TidyMetaTask --target TidyMeta_For_CI
        DEPENDS  thisMetaTidy.txt
    )
endif()
