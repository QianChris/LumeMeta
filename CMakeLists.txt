#
# Copyright (C) 2022 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.19)

project (LumeMeta LANGUAGES CXX)

set(LUMEBASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/submodules/LumeBase CACHE PATH "Path to lume base sources")
set(LUMEENGINE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/submodules/LumeEngine CACHE PATH "Path to lume engine sources")
set(CORE_UNPACKED_LIB_DIR ${CMAKE_BINARY_DIR}/unpacked CACHE PATH "Path where to extract 3rd party library sources.")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# do not add /GR to MSVC builds by default
cmake_policy(SET CMP0117 NEW)

#
# Options
#
option(META_VISUAL_STUDIO_TIDY  "Create hackish targets to run clang-tidy steps in visual studio" OFF)
option(META_SUBMODULE_ENABLED "Use ${PROJECT_NAME} as submodule" OFF)
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
option(META_UNIT_TESTS_ENABLED "Build ${PROJECT_NAME} unit tests" ON)
if(META_UNIT_TESTS_ENABLED)
    option(META_UNIT_TESTS_WITH_SANITIZERS "Build ${PROJECT_NAME} unit tests with sanitizers" OFF)
else()
    unset(META_UNIT_TESTS_WITH_SANITIZERS CACHE)
endif()
option(META_BENCHMARK_TESTS_ENABLED "Build ${PROJECT_NAME} benchmark tests" OFF)

#
# Lib config.
#
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

#
# API and Plugin projects
#
add_subdirectory(include)
add_subdirectory(src)

#
# Testing projects
#
if(META_UNIT_TESTS_ENABLED)
    # API unit tests
    add_subdirectory(test/api_unit_test)
endif()

if(META_BENCHMARK_TESTS_ENABLED AND NOT ANDROID AND NOT OHOS)
    # Benchmark tests
    add_subdirectory(test/benchmark)
endif()

#
# Clang-tidy
#
if (META_VISUAL_STUDIO_TIDY AND NOT META_SUBMODULE_ENABLED)
    # Somewhat hackish way to run the clang-tidy step from visual studio...
    message("Creating VS ExecTidyMeta in ${PROJECT_NAME} cmakelists.txt")
    add_custom_target(ExecTidyMeta DEPENDS thatMetaTidy.txt
        OUTPUT  thatMetaTidy.txt
        COMMAND ${CMAKE_COMMAND} -DCORE_UNPACKED_LIB_DIR=${CORE_UNPACKED_LIB_DIR} -DPACKAGE_CACHE_PATH=${PACKAGE_CACHE_PATH} -DLUMEBASE_PATH=${LUMEBASE_PATH} -DLUMEENGINE_PATH=${LUMEENGINE_PATH} -DMETA_UNIT_TESTS_ENABLED=${META_UNIT_TESTS_ENABLED} -DMETA_BENCHMARK_TESTS_ENABLED=${META_BENCHMARK_TESTS_ENABLED} -DBUILD_DEPS=${BUILD_DEPS} -DBUILD_LUME_CORE_LIBS=${BUILD_LUME_CORE_LIBS} -DLUME_META_DISABLE_RUNTIME_DEPENDENCIES=true -DLUME_META_OBJECT_PERFORM_CODE_INSPECTION=true -G "Ninja" -DCMAKE_BUILD_TYPE="Release" -S ${CMAKE_CURRENT_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/TidyMetaTask
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/TidyMetaTask --target TidyMeta_For_CI
        DEPENDS  thisMetaTidy.txt
    )
endif()
