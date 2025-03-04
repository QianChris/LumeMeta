/**
 *  Copyright (C) 2023 Huawei Technologies Co, Ltd.
 */
#include <tester.h>
#include <testing_objects.h>

#include <gtest/gtest.h>

#include <meta/api/property/property_info.h>
#include <meta/interface/detail/enum.h>
#include <meta/interface/enum_macros.h>
#include <meta/interface/property/construct_property.h>

using namespace testing;

META_BEGIN_NAMESPACE()
enum class MyPropEnum {
    A = 1,
    B = 2,
};

META_BEGIN_ENUM(MyPropEnum, "Nice Enum", MyPropEnum::A)
META_ENUM_VALUE(MyPropEnum::A, "MyA", "My A enumerator")
META_ENUM_VALUE(MyPropEnum::B, "MyB", "My B enumerator")
META_END_ENUM()

META_ENUM_TYPE(MyPropEnum)

namespace Test {

TEST(PropertyInfoTest, Info)
{
    auto p = ConstructProperty<MyPropEnum>("test");
    PropertyInfo info(p);
    ASSERT_TRUE(info);
    EXPECT_EQ(info->GetName(), "MyPropEnum");
    EXPECT_EQ(info->GetDescription(), "Nice Enum");
}

TEST(PropertyInfoTest, ConstEnum)
{
    auto p = ConstructProperty<MyPropEnum>("test");
    ConstPropertyEnumInfo info(p);
    ASSERT_TRUE(info);
    EXPECT_EQ(info->GetName(), "MyPropEnum");
    EXPECT_EQ(info->GetDescription(), "Nice Enum");
    EXPECT_EQ(info->GetEnumType(), EnumType::SINGLE_VALUE);
    EXPECT_EQ(info->GetValueIndex(), 0);
    EXPECT_EQ(info->GetValues().size(), 2);
}

TEST(PropertyInfoTest, Enum)
{
    auto p = ConstructProperty<MyPropEnum>("test");
    PropertyEnumInfo info(p);
    ASSERT_TRUE(info);
    EXPECT_TRUE(info->SetValueIndex(1));
    EXPECT_EQ(info->GetValueIndex(), 1);
    EXPECT_EQ(p->GetValue(), MyPropEnum::B);
}

} // namespace Test

META_END_NAMESPACE()
