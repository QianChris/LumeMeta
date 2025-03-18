cmake_minimum_required(VERSION 3.19)
if (NOT TARGET AGPEngine::ComponentHelperSDK)
    get_filename_component(root_path "${CMAKE_CURRENT_LIST_DIR}/../" ABSOLUTE)
    add_library(AGPEngine::ComponentHelperSDK INTERFACE IMPORTED GLOBAL)
    set_property(TARGET AGPEngine::ComponentHelperSDK PROPERTY CXX_STANDARD 17)    
    target_include_directories(AGPEngine::ComponentHelperSDK INTERFACE "${root_path}/ecshelper")
    target_sources(AGPEngine::ComponentHelperSDK INTERFACE
        ${root_path}/ecshelper/ComponentTools/base_manager.inl
        ${root_path}/ecshelper/ComponentTools/base_manager.h
        ${root_path}/ecshelper/ComponentTools/component_query.h
        ${root_path}/ecshelper/ComponentTools/component_query.cpp
    )
endif()
set(ComponentHelperSDK_FOUND true)