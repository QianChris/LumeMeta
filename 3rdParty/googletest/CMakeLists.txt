#
# Copyright (C) 2018 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.19)

include(../unpack.cmake)
#
# googletest
#

# Decompress the library archive to the build dir if not already done.
Unpack(
    "${CMAKE_CURRENT_LIST_DIR}/googletest-release-1.12.0.tar.gz"
    "${CORE_UNPACKED_LIB_DIR}"
    "googletest-release-1.12.0"
    googletest.unpacked.dir
)

# Disable extra gtest stuff by default.
set(INSTALL_GTEST OFF CACHE BOOL "")
set(BUILD_GMOCK ON CACHE BOOL "")
# CRT linking options need to match. See https://github.com/google/googletest/blob/master/googletest/README.md
set(gtest_force_shared_crt ON CACHE BOOL "")

add_subdirectory(${googletest.unpacked.dir} googletest EXCLUDE_FROM_ALL)
set_target_properties(gtest gtest_main gmock gmock_main PROPERTIES FOLDER "3rdparty")

