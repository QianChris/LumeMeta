# To enable code inspection set flag LUME_META_OBJECT_PERFORM_CODE_INSPECTION as ON.
# To run code inspection for given target just use meta_object_target_code_inspection function with target name
# meta_object_target_code_inspection(TargetName) and build this target.
# Build all to inspect all targets that meta_object_target_code_inspection was used on.
#
# Important: CXX_CLANG_TIDY property works only for Ninja and Makefile Generators
#
# Code inspection is performed by clang-tidy. To use it on Windows go to Visual Studio Installer and install packages:
# - C++ Clang tools for Windows
# - C++ Clang-cl for v143 (or different version) build tools
# - C++ Clang compiler for Windows (14.0.5)

option(LUME_META_OBJECT_PERFORM_CODE_INSPECTION "If ON code inspection is performed during build" OFF)

function(meta_object_target_code_inspection TARGET_NAME)
    if (NOT $CACHE{LUME_META_OBJECT_PERFORM_CODE_INSPECTION})
        return()
    endif ()

    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "There is not such target \"${TARGET_NAME}\"")
    endif ()

    if (NOT CMAKE_GENERATOR STREQUAL "Ninja" AND NOT CMAKE_GENERATOR MATCHES "Makefiles")
        message(SEND_ERROR "Code inspection can be performed only for Ninja and Makefiles generators")
    endif()

    find_program(CLANG_TIDY
            NAMES clang-tidy
            REQUIRED
            NO_CACHE)

    set(CLANG_TIDY_COMMAND ${CLANG_TIDY} --use-color)
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")

    if (NOT CLANG_TIDY_INFO_PRINTED)
        message(STATUS "clang-tidy found: ${CLANG_TIDY}")

        message("Checking version of clang tidy")
        execute_process(COMMAND ${CLANG_TIDY} --version)
        set(CLANG_TIDY_INFO_PRINTED ON CACHE INTERNAL "")
    endif()

    if (NOT TARGET TidyMeta_For_CI)
        add_custom_target(TidyMeta_For_CI
            COMMAND echo TidyMeta_For_CI
            )
    endif()
    add_dependencies(TidyMeta_For_CI ${TARGET_NAME})
endfunction()
