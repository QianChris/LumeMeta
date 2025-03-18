/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Unit tests for TimeSpan
 * Author: Lauri Jaaskela
 * Create: 2022-10-07
 */

#include <test_utils.h>

#include <gtest/gtest.h>

#include <meta/base/time_span.h>

META_BEGIN_NAMESPACE()

namespace Test {

using namespace META_NS::TimeSpanLiterals;

TEST(TimeSpan, InitMethods)
{
    constexpr auto v1 = TimeSpan::Milliseconds(2000);
    constexpr auto v2 = TimeSpan::Microseconds(2000000);
    constexpr auto v3 = TimeSpan::Seconds(2);
    constexpr TimeSpan v4 = 2_s;
    constexpr auto v5 = 2000_ms;
    constexpr auto v6 = 2000000_us;

    // Should be same regardless of initialization method
    EXPECT_EQ(v1, v2);
    EXPECT_EQ(v2, v3);
    EXPECT_EQ(v4, v5);
    EXPECT_EQ(v5, v6);
    EXPECT_EQ(v6, v1);
}

TEST(TimeSpan, Addition)
{
    constexpr auto v1 = TimeSpan::Milliseconds(400);
    constexpr auto v2 = TimeSpan::Microseconds(2000);

    constexpr auto v3 = v1 + v2;

    EXPECT_EQ(TimeSpan::Milliseconds(402), v3);

    // Check that v1 and v2 haven't changed
    EXPECT_EQ(TimeSpan::Milliseconds(400), v1);
    EXPECT_EQ(TimeSpan::Microseconds(2000), v2);
}

TEST(TimeSpan, Subtraction)
{
    constexpr auto v1 = TimeSpan::Milliseconds(400);
    constexpr auto v2 = TimeSpan::Microseconds(2000);

    constexpr auto v3 = v1 - v2;

    EXPECT_EQ(TimeSpan::Milliseconds(398), v3);

    // Check that v1 and v2 haven't changed
    EXPECT_EQ(TimeSpan::Milliseconds(400), v1);
    EXPECT_EQ(TimeSpan::Microseconds(2000), v2);
}

TEST(TimeSpan, Negation)
{
    constexpr auto v1 = -TimeSpan::Milliseconds(200);
    constexpr auto v2 = -200_ms;

    EXPECT_EQ(v1, v2);
    EXPECT_EQ(v1, TimeSpan::Milliseconds(-200));
    EXPECT_EQ(v2, TimeSpan::Milliseconds(-200));
}

TEST(TimeSpan, Infinities)
{
    EXPECT_EQ(TimeSpan::Infinite(), TimeSpan::Infinite() + TimeSpan::Infinite());
    EXPECT_EQ(-TimeSpan::Infinite(), -TimeSpan::Infinite() - TimeSpan::Infinite());
    EXPECT_EQ(TimeSpan::Infinite(), 3 * TimeSpan::Infinite());
    EXPECT_EQ(-TimeSpan::Infinite(), -1 * TimeSpan::Infinite());
    EXPECT_EQ(TimeSpan::Infinite(), -1 * -TimeSpan::Infinite());
    EXPECT_EQ(TimeSpan::Zero(), 0 * TimeSpan::Infinite());
    EXPECT_EQ(TimeSpan::Zero(), 0 * -TimeSpan::Infinite());

    EXPECT_EQ(TimeSpan::Infinite(), TimeSpan::Infinite() + TimeSpan::Microseconds(10));
    EXPECT_EQ(TimeSpan::Infinite(), TimeSpan::Infinite() - TimeSpan::Microseconds(10));
    EXPECT_EQ(-TimeSpan::Infinite(), -TimeSpan::Infinite() + TimeSpan::Microseconds(10));
    EXPECT_EQ(-TimeSpan::Infinite(), -TimeSpan::Infinite() - TimeSpan::Microseconds(10));

    auto v1 = TimeSpan::Infinite();
    v1 += TimeSpan::Milliseconds(10);

    EXPECT_EQ(TimeSpan::Infinite(), v1);

    auto v2 = TimeSpan::Infinite();
    v2 -= TimeSpan::Milliseconds(10);

    EXPECT_EQ(TimeSpan::Infinite(), v2);
}

TEST(TimeSpan, TimeIsRoundedToMicroseconds)
{
    EXPECT_EQ(TimeSpan::Microseconds(1), TimeSpan::Seconds(1e-6));
    EXPECT_EQ(TimeSpan::Microseconds(0), TimeSpan::Seconds(0.4e-6));
    EXPECT_EQ(TimeSpan::Microseconds(1), TimeSpan::Seconds(0.5e-6));

    auto timeSpan = TimeSpan::Microseconds(1);
    timeSpan.SetSeconds(0.4e-6);
    EXPECT_EQ(TimeSpan::Microseconds(0), timeSpan);

    timeSpan.SetSeconds(0.5e-6);
    EXPECT_EQ(TimeSpan::Microseconds(1), timeSpan);
}

TEST(TimeSpan, MultiplicationResultIsRoundedToMicroseconds)
{
    EXPECT_EQ(TimeSpan::Microseconds(1), TimeSpan::Microseconds(1) * 0.5f);
    EXPECT_EQ(TimeSpan::Microseconds(0), TimeSpan::Microseconds(1) * 0.1f);
}

TEST(TimeSpan, DivisionResultIsRoundedToMicroseconds)
{
    EXPECT_EQ(TimeSpan::Microseconds(1), TimeSpan::Microseconds(1) / 2);
    EXPECT_EQ(TimeSpan::Microseconds(0), TimeSpan::Microseconds(1) / 10);
    EXPECT_EQ(TimeSpan::Microseconds(1), TimeSpan::Microseconds(1) / 2.f);
    EXPECT_EQ(TimeSpan::Microseconds(0), TimeSpan::Microseconds(1) / 10.f);
}

} // namespace Test
META_END_NAMESPACE()
