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

#include <test_framework.h>

#include <meta/interface/simple_event.h>

META_BEGIN_NAMESPACE()
namespace UTest {
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

UNIT_TEST(API_EventTest, CreateSimpleEvent, testing::ext::TestSize.Level1)
{
    EXPECT_EQ(Event1::UID, test1Info::UID);
    EXPECT_EQ(Event2::UID, test2Info::UID);
}

} // namespace UTest
META_END_NAMESPACE()
