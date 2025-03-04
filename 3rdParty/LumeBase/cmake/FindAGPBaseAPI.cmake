cmake_minimum_required(VERSION 3.19)
if (NOT TARGET AGPBase::AGPBaseAPI )
    find_package(securec REQUIRED)
    get_filename_component(root_path "${CMAKE_CURRENT_LIST_DIR}/../" ABSOLUTE)

    set(BASE_ROOT_DIRECTORY ${root_path} CACHE PATH "Path to AGP Base" FORCE)
    add_library(AGPBase::AGPBaseAPI INTERFACE IMPORTED GLOBAL)
    set_property(TARGET AGPBase::AGPBaseAPI PROPERTY CXX_STANDARD 17)
    target_compile_definitions(AGPBase::AGPBaseAPI INTERFACE "\$<\$<CONFIG:Release>:BASE_LOG_NO_DEBUG=1>;\$<\$<CONFIG:MinSizeRel>:BASE_LOG_NO_DEBUG=1>;\$<\$<CONFIG:RelWithDebInfo>:BASE_LOG_NO_DEBUG=1>")
    target_include_directories(AGPBase::AGPBaseAPI INTERFACE ${root_path}/api/)
    target_link_libraries(AGPBase::AGPBaseAPI INTERFACE securec::securec)

    #this is a bit hackish.. need to check the core and base "cmake_tools.cmake"
    #and see what we want to force as "api"
    if (MSVC_VERSION GREATER_EQUAL 1915 AND NOT CMAKE_GENERATOR STREQUAL "Ninja")
        #disable warning C4577: 'noexcept' used with no exception handling mode specified; ....
        target_compile_options(AGPBase::AGPBaseAPI INTERFACE /wd4577)
        #disable warning C4530: C++ exception handler used, but unwind semantics are not enabled.
        target_compile_options(AGPBase::AGPBaseAPI INTERFACE /wd4530)
    endif()
endif()
set(AGPBaseAPI_FOUND true)