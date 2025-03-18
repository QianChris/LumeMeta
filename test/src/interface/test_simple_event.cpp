/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#include <gtest/gtest.h>

#include <meta/interface/simple_event.h>

META_BEGIN_NAMESPACE()
namespace Test {
namespace {

constexpr BASE_NS::Uid uid1 { "234b88b1-84db-46c2-b837-d294d9f806a2" };
constexpr BASE_NS::Uid uid2 { "637b87b2-845b-4652-b857-d29459f806a2" };

struct test1Info {
    constexpr static BASE_NS::Uid UID { uid1 };
    constexpr static char const* NAME { "Test" };
};

struct test2Info {
    constexpr static BASE_NS::Uid UID { uid2 };
    constexpr static char const* NAME { "Test" };
};

using Event1 = SimpleEvent<test1Info>;
using Event2 = SimpleEvent<test2Info, void(int, int)>;

} // namespace

TEST(EventTest, CreateSimpleEvent)
{
    EXPECT_EQ(Event1::UID, test1Info::UID);
    EXPECT_EQ(Event2::UID, test2Info::UID);
}

} // namespace Test
META_END_NAMESPACE()
