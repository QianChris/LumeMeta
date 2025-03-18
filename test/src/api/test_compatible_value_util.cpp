/**
 *  Copyright (C) 2023 Huawei Technologies Co, Ltd.
 */
#include <tester.h>
#include <testing_objects.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/api/compatible_value_util.h>
#include <meta/interface/detail/any.h>

using namespace testing;

META_BEGIN_NAMESPACE()

namespace Test {

using types = TypeList<bool, char, int16_t, int32_t>;

TEST(CompatibleValueUtilTest, SetGet)
{
    Any<int16_t> any { 2 };

    int64_t v = 0;
    EXPECT_TRUE(GetCompatibleValue(any, v, types {}));
    EXPECT_EQ(v, 2);

    size_t s = 3;
    EXPECT_TRUE(SetCompatibleValue(s, any, types {}));
    EXPECT_EQ(any.InternalGetValue(), 3);
}

} // namespace Test

META_END_NAMESPACE()
