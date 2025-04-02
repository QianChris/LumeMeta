/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <meta/base/version.h>

META_BEGIN_NAMESPACE()
namespace UTest {

TEST(API_VersionTest, Compare)
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

TEST(API_VersionTest, ToString)
{
    TestFromToVersion(Version());
    TestFromToVersion(Version(1, 0));
    TestFromToVersion(Version(1, 1));
    TestFromToVersion(Version(1, 9999));
    TestFromToVersion(Version(9999, 0));
    TestFromToVersion(Version(123, 2));
}

TEST(API_VersionTest, FromString)
{
    EXPECT_EQ(Version("1"), Version(1, 0));
    EXPECT_EQ(Version("1125"), Version(1125, 0));
    EXPECT_EQ(Version("1."), Version(1, 0));
    EXPECT_EQ(Version("1.df"), Version(1, 0));
}

TEST(API_VersionTest, InvalidString)
{
    EXPECT_EQ(Version(""), Version());
    EXPECT_EQ(Version(".1"), Version());
    EXPECT_EQ(Version(".."), Version());
    EXPECT_EQ(Version("asdb"), Version());
    EXPECT_EQ(Version("-1.24"), Version());
}

} // namespace UTest
META_END_NAMESPACE()
