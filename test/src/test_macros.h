/**
 *  Copyright (C) 2023 Huawei Technologies Co, Ltd.
 */
#ifndef META_TEST_MACROS_HEADER
#define META_TEST_MACROS_HEADER

#include <chrono>

#include <gtest/gtest.h>

#define META_WAIT_TIMED(millis) std::this_thread::sleep_for(std::chrono::milliseconds(millis));

#define META_WAIT_TRUE_TIMED(millis, arg)                                                \
    {                                                                                    \
        auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(millis); \
        while (!(arg) && std::chrono::steady_clock::now() < end) {                       \
            std::this_thread::sleep_for(std::chrono::milliseconds(1));                   \
        }                                                                                \
    }

#define EXPECT_TRUE_TIMED(millis, arg) \
    META_WAIT_TRUE_TIMED(millis, arg); \
    EXPECT_TRUE(arg);

#define EXPECT_EQ_TIMED(millis, arg, value)         \
    META_WAIT_TRUE_TIMED(millis, (arg) == (value)); \
    EXPECT_EQ(arg, value);

#define EXPECT_THAT_TIMED(millis, arg, matcher)                     \
    META_WAIT_TRUE_TIMED(millis, (testing::Matches(matcher)(arg))); \
    EXPECT_THAT(arg, matcher);

#endif
