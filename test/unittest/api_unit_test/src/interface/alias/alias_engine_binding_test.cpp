/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

// clang-format off
#include "../engine/engine_test_property.h"
// clang-format on

#include <test_framework.h>

#include <core/property/property_handle_util.h>

#include <meta/interface/alias/intf_alias_registry.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/engine/intf_engine_value_manager.h>
#include <meta/interface/intf_object_registry.h>

META_BEGIN_NAMESPACE()
namespace UTest {

// A binding sits on top of a consumer property and does not write down into the engine value beneath
// it, so a cell change would never reach the engine on its own. The registry pushes it explicitly.
// These cover that contract; that the pushed value then reaches a real ECS component is LumeScene's to
// prove, since only it has one.
class API_AliasEngineBindingTest : public ::testing::Test {
public:
    IAliasRegistry::Ptr MakeRegistry()
    {
        return GetObjectRegistry().Create<IAliasRegistry>(ClassId::AliasRegistry);
    }
};

/**
 * @tc.name: DefineReachesTheEngineValue
 * @tc.desc: Defining an alias drives a bound engine-backed property all the way down to the engine
 *           value, so the next sync writes it into the component data.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasEngineBindingTest, DefineReachesTheEngineValue, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop1::EngineTestProp> cman{prop1::ENGINE_TESTPROP_METADATA};
    auto manager = GetObjectRegistry().Create<IEngineValueManager>(ClassId::EngineValueManager);
    ASSERT_TRUE(manager);
    ASSERT_TRUE(manager->ConstructValues(EnginePropertyHandle{&cman, cman.entityRef}));
    auto p = manager->ConstructProperty<int32_t>("value");
    ASSERT_TRUE(p);

    auto provider = MakeRegistry();
    ASSERT_TRUE(provider->Bind(p.GetProperty(), "v"));
    EXPECT_TRUE(provider->Define("v", *ConstructAny<int32_t>(7)));
    EXPECT_EQ(p->GetValue(), 7);

    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(CORE_NS::GetPropertyValue<int32_t>(*cman.GetData(0), "value"), 7);
}

/**
 * @tc.name: UndefineReachesTheEngineValue
 * @tc.desc: Falling back to the alias default is a change like any other and is pushed down too, so an
 *           alias that stops supplying a value does not leave the engine holding the old one.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasEngineBindingTest, UndefineReachesTheEngineValue, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop1::EngineTestProp> cman{prop1::ENGINE_TESTPROP_METADATA};
    auto manager = GetObjectRegistry().Create<IEngineValueManager>(ClassId::EngineValueManager);
    ASSERT_TRUE(manager);
    ASSERT_TRUE(manager->ConstructValues(EnginePropertyHandle{&cman, cman.entityRef}));
    auto p = manager->ConstructProperty<int32_t>("value");
    ASSERT_TRUE(p);

    auto provider = MakeRegistry();
    ASSERT_TRUE(provider->SetDefault("v", ConstructAny<int32_t>(3)));
    ASSERT_TRUE(provider->Bind(p.GetProperty(), "v"));
    EXPECT_TRUE(provider->Define("v", *ConstructAny<int32_t>(9)));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(CORE_NS::GetPropertyValue<int32_t>(*cman.GetData(0), "value"), 9);

    EXPECT_TRUE(provider->Undefine("v"));
    EXPECT_EQ(p->GetValue(), 3);
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(CORE_NS::GetPropertyValue<int32_t>(*cman.GetData(0), "value"), 3);
}

/**
 * @tc.name: RebindStopsThePreviousAliasPushing
 * @tc.desc: A property follows at most one alias, so the one it left must stop writing into its engine
 *           value — otherwise a stale alias keeps overwriting the component behind the new one's back.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasEngineBindingTest, RebindStopsThePreviousAliasPushing, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop1::EngineTestProp> cman{prop1::ENGINE_TESTPROP_METADATA};
    auto manager = GetObjectRegistry().Create<IEngineValueManager>(ClassId::EngineValueManager);
    ASSERT_TRUE(manager);
    ASSERT_TRUE(manager->ConstructValues(EnginePropertyHandle{&cman, cman.entityRef}));
    auto p = manager->ConstructProperty<int32_t>("value");
    ASSERT_TRUE(p);

    auto provider = MakeRegistry();
    ASSERT_TRUE(provider->Bind(p.GetProperty(), "a"));
    EXPECT_TRUE(provider->Define("a", *ConstructAny<int32_t>(1)));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(CORE_NS::GetPropertyValue<int32_t>(*cman.GetData(0), "value"), 1);

    // Following "b" now; "a" no longer counts it as a consumer.
    ASSERT_TRUE(provider->Bind(p.GetProperty(), "b"));
    EXPECT_EQ(provider->GetConsumerCount("a"), 0u);
    EXPECT_TRUE(provider->Define("b", *ConstructAny<int32_t>(2)));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(CORE_NS::GetPropertyValue<int32_t>(*cman.GetData(0), "value"), 2);

    // Redefining the alias it left must not reach the engine value.
    EXPECT_TRUE(provider->Define("a", *ConstructAny<int32_t>(99)));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(CORE_NS::GetPropertyValue<int32_t>(*cman.GetData(0), "value"), 2);
}

}  // namespace UTest
META_END_NAMESPACE()
