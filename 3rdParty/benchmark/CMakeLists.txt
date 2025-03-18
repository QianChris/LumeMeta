#
# Copyright (C) 2022 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.19)

include(../unpack.cmake)

# Decompress the library archive to the build dir if not already done.
Unpack(
    "${CMAKE_CURRENT_LIST_DIR}/benchmark-1.6.2.tar.gz"
    "${CORE_UNPACKED_LIB_DIR}"
    "benchmark-1.6.2"
    benchmark.unpacked.dir
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "")
set(HAVE_GNU_POSIX_REGEX 0 CACHE INTERNAL "" FORCE)
set(HAVE_POSIX_REGEX 0 CACHE INTERNAL "" FORCE)

add_subdirectory(${benchmark.unpacked.dir} benchmark EXCLUDE_FROM_ALL)
set_target_properties(benchmark benchmark_main PROPERTIES FOLDER "3rdparty")

