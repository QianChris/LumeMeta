# Adds to target dependency for plugin and forces to update plugin
# target_plugins_dependencies(targetName
#                             PLUGINS plugin1 [plugin2 ...]
#                             [FULL_DEPLOY_PATH deployDirectoryPath]
#                             [RELATIVE_DEPLOY_PATH subdirectoryPostfix]
#                             VERBOSE
#                             )

function(target_plugins_dependencies TARGET_NAME)
    if ($CACHE{LUME_META_DISABLE_RUNTIME_DEPENDENCIES})
        # when doing CODE_INSPECTION (clang-tidy) on CI
        # we don't actually need the runtime binaries.
        # so ignore the deploy steps.
        return()
    endif ()
    set(OPTIONS_LIST VERBOSE)
    set(SINGLE_ARG_LIST FULL_DEPLOY_PATH RELATIVE_DEPLOY_PATH)
    set(MULTI_ARG_LIST PLUGINS)

    cmake_parse_arguments(PREFIX "${OPTIONS_LIST}" "${SINGLE_ARG_LIST}" "${MULTI_ARG_LIST}" ${ARGN})

    # Checking if target name is empty or not exists
    if (NOT TARGET_NAME OR NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "Cannot find target ${TARGET_NAME}")
    endif ()

    if (NOT PREFIX_PLUGINS)
        message(FATAL_ERROR "Please provide at least one plugin (Use PLUGINS option for that)")
    endif ()

    foreach (PLUGIN_TARGET IN LISTS PREFIX_PLUGINS)
        if (NOT TARGET ${PLUGIN_TARGET})
            message(FATAL_ERROR "There is no plug- target \"${PLUGIN_TARGET}\"")
        endif ()

        get_target_property(TARGET_TYPE ${PLUGIN_TARGET} TYPE)

        # plugins library type usually is MODULE, but currently most of plugins targets type is SHARED,
        # so for now we consider SHARED_LIBRARY also
        if (NOT TARGET_TYPE STREQUAL "MODULE_LIBRARY"
                AND NOT TARGET_TYPE STREQUAL "SHARED_LIBRARY")
            message(FATAL_ERROR "Target ${TARGET_TYPE} is not a plugin. It's type is \"${TARGET_TYPE}\"")
        endif ()

        add_dependencies(${TARGET_NAME} ${PLUGIN_TARGET})

        if (NOT PREFIX_RELATIVE_DEPLOY_PATH AND NOT PREFIX_FULL_DEPLOY_PATH)
            continue()
        endif ()

        # we replace "::" in case of target has namespace, because paths cannot contain colons
        string(REPLACE "::" "-" DEPENDENCY_FILE_BASE_NAME ${PLUGIN_TARGET})

        if (PREFIX_RELATIVE_DEPLOY_PATH)
            if (PREFIX_VERBOSE)
                set(UPDATING_MESSAGE "Deploying ${PLUGIN_TARGET} to \"${PREFIX_RELATIVE_DEPLOY_PATH}\" directory")
            endif ()

            set(RELATIVE_DEPLOY_DEPENDENCY_FILE "${CMAKE_CURRENT_BINARY_DIR}/${DEPENDENCY_FILE_BASE_NAME}-relative-path-deploy.txt")
            target_sources(${TARGET_NAME} PRIVATE ${RELATIVE_DEPLOY_DEPENDENCY_FILE})
            source_group("CMake Rules" FILES ${RELATIVE_DEPLOY_DEPENDENCY_FILE})
            add_custom_command(OUTPUT ${RELATIVE_DEPLOY_DEPENDENCY_FILE}
                    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${TARGET_NAME}>/${PREFIX_RELATIVE_DEPLOY_PATH}/
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PLUGIN_TARGET}> $<TARGET_FILE_DIR:${TARGET_NAME}>/${PREFIX_RELATIVE_DEPLOY_PATH}/
                    COMMENT "${UPDATING_MESSAGE}"
                    )
        endif ()

        if (PREFIX_FULL_DEPLOY_PATH)
            if (PREFIX_VERBOSE)
                set(UPDATING_MESSAGE "Deploying ${PLUGIN_TARGET} to ${PREFIX_FULL_DEPLOY_PATH} directory")
            endif ()

            set(FULL_PATH_DEPLOY_DEPENDENCY_FILE "${CMAKE_CURRENT_BINARY_DIR}/${DEPENDENCY_FILE_BASE_NAME}-absolute-path-deploy.txt")
            target_sources(${TARGET_NAME} PRIVATE ${FULL_PATH_DEPLOY_DEPENDENCY_FILE})
            source_group("CMake Rules" FILES ${FULL_PATH_DEPLOY_DEPENDENCY_FILE})
            add_custom_command(OUTPUT ${FULL_PATH_DEPLOY_DEPENDENCY_FILE}
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${PREFIX_FULL_DEPLOY_PATH}/
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PLUGIN_TARGET}> ${PREFIX_FULL_DEPLOY_PATH}
                    COMMENT "${UPDATING_MESSAGE}"
                    )
        endif ()
    endforeach ()
endfunction()
