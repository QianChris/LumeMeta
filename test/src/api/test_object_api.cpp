/**
 *  Copyright (C) 2023 Huawei Technologies Co, Ltd.
 */
#include <tester.h>
#include <testing_objects.h>

#include <gtest/gtest.h>

#include <meta/api/object.h>
#include <meta/interface/property/property.h>

#include "../testing_objects.h"
#include "../util.h"

using namespace testing;

META_BEGIN_NAMESPACE()

namespace Test {

class TestInterfaceAPI : public Internal::ObjectInterfaceAPI<TestInterfaceAPI, ClassId::TestType> {
    META_API(TestInterfaceAPI)
    META_API_OBJECT_CONVERTIBLE(ITestType)
    META_API_CACHE_INTERFACE(ITestType, Test)

public:
    META_API_INTERFACE_PROPERTY_CACHED(Test, First, int)
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Test, Third, int)
    META_API_INTERFACE_ARRAY_PROPERTY_CACHED(Test, MyIntArray, int)
    META_API_INTERFACE_READONLY_ARRAY_PROPERTY_CACHED(Test, MyConstIntArray, int)
};

TEST(ObjectApiTest, Properties)
{
    TestInterfaceAPI test;
    test.First(1);
    EXPECT_EQ(test.First()->GetValue(), 1);
    EXPECT_EQ(test.Third()->GetValue(), 0);
    test.MyIntArray({ 1, 2 });
    EXPECT_EQ(test.MyIntArray()->GetValue(), (BASE_NS::vector<int> { 1, 2 }));
    EXPECT_EQ(test.MyConstIntArray()->GetValue(), (BASE_NS::vector<int> { 1, 2, 3, 4, 5 }));
    test.First(GetValue(test.Third()));
    EXPECT_EQ(test.First()->GetValue(), 0);
    test.MyIntArray(test.MyConstIntArray()->GetValue());
    EXPECT_EQ(test.MyIntArray()->GetValue(), (BASE_NS::vector<int> { 1, 2, 3, 4, 5 }));
}

} // namespace Test

META_END_NAMESPACE()
