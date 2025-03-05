/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#include <gtest/gtest.h>

#include <meta/base/version.h>

META_BEGIN_NAMESPACE()
namespace Test {

TEST(VersionTest, Compare)
{
    Version a;
    Version b(1, 0);
    Version c(1, 1);
    Version d(0, 9999);
    Version e(1, 9999);
    Version f(2366, 0);

    EXPECT_TRUE(a == a);
    EXPECT_TRUE(b == b);
    EXPECT_TRUE(a != b);

    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b < c);
    EXPECT_TRUE(d < b);
    EXPECT_TRUE(e < f);

    EXPECT_FALSE(b > b);
    EXPECT_FALSE(b > c);
    EXPECT_FALSE(d > b);
    EXPECT_FALSE(e > f);

    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(b <= b);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(b <= c);
    EXPECT_TRUE(d <= b);
    EXPECT_TRUE(e <= f);

    EXPECT_TRUE(a >= a);
    EXPECT_TRUE(b >= b);
    EXPECT_FALSE(a >= b);
    EXPECT_FALSE(b >= c);
    EXPECT_FALSE(d >= b);
    EXPECT_FALSE(e >= f);
}

void TestFromToVersion(Version v)
{
    Version other(v.ToString());
    EXPECT_EQ(v, other);
}

TEST(VersionTest, ToString)
{
    TestFromToVersion(Version());
    TestFromToVersion(Version(1, 0));
    TestFromToVersion(Version(1, 1));
    TestFromToVersion(Version(1, 9999));
    TestFromToVersion(Version(9999, 0));
    TestFromToVersion(Version(123, 2));
}

TEST(VersionTest, FromString)
{
    EXPECT_EQ(Version("1"), Version(1, 0));
    EXPECT_EQ(Version("1125"), Version(1125, 0));
    EXPECT_EQ(Version("1."), Version(1, 0));
    EXPECT_EQ(Version("1.df"), Version(1, 0));
}

TEST(VersionTest, InvalidString)
{
    EXPECT_EQ(Version(""), Version());
    EXPECT_EQ(Version(".1"), Version());
    EXPECT_EQ(Version(".."), Version());
    EXPECT_EQ(Version("asdb"), Version());
    EXPECT_EQ(Version("-1.24"), Version());
}

} // namespace Test
META_END_NAMESPACE()
