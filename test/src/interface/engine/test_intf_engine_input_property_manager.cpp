/**
 *  Copyright (C) 2023 Huawei Technologies Co, Ltd.
 */
// clang-format off
#include "engine_test_property.h"
// clang-format on

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <core/property/property_handle_util.h>

#include <meta/interface/builtin_objects.h>
#include <meta/interface/engine/intf_engine_input_property_manager.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_registry.h>

#include "tester.h"
#include "testing_objects.h"
#include "util.h"

using namespace testing;

META_BEGIN_NAMESPACE()
namespace Test {

class EngineInputPropertyManagerTest : public ::testing::Test {
public:
    void SetUp() override
    {
        engine_ = getTestEnv()->engine;
        RegisterEngineTestType();
    }
    void TearDown() override
    {
        UnregisterEngineTestType();
    }

private:
    CORE_NS::IEngine* engine_;
};

TEST_F(EngineInputPropertyManagerTest, Property)
{
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };

    auto manager =
        GetObjectRegistry().Create<IEngineInputPropertyManager>(META_NS::ClassId::EngineInputPropertyManager);
    ASSERT_TRUE(manager);

    EXPECT_TRUE(manager->GetValueManager()->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));

    auto p = manager->ConstructProperty<int32_t>("value");
    ASSERT_TRUE(p);

    p->SetValue(2);
    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 0));
    EXPECT_TRUE(manager->Sync());
    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 2));

    auto source = ConstructProperty<int32_t>("source");
    p->SetBind(source);
    source->SetValue(3);
    EXPECT_TRUE(manager->Sync());
    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 3));

    auto other = ConstructProperty<int32_t>("source");
    manager->TieProperty(other, "value");
    p->SetValue(1);
    EXPECT_TRUE(manager->Sync());
    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 3));

    other->SetValue(7);
    EXPECT_TRUE(manager->Sync());
    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 7));
}

TEST_F(EngineInputPropertyManagerTest, PopulateMetadata)
{
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };

    auto manager =
        GetObjectRegistry().Create<IEngineInputPropertyManager>(META_NS::ClassId::EngineInputPropertyManager);
    ASSERT_TRUE(manager);
    EXPECT_TRUE(manager->GetValueManager()->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));

    auto m = GetObjectRegistry().Create<IMetadata>(ClassId::Object);
    ASSERT_TRUE(m);

    EXPECT_TRUE(manager->PopulateProperties(*m));

    auto cont = interface_cast<IAttach>(m)->GetAttachmentContainer(true);
    EXPECT_EQ(cont->GetSize(), prop1::TEST_PROPERTY_COUNT);
    auto p = m->GetProperty<int32_t>("value");
    ASSERT_TRUE(p);
    auto fp = m->GetArrayProperty<float>("floats");
    ASSERT_TRUE(fp);

    p->SetValue(2);
    EXPECT_TRUE(manager->Sync());
    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 2));
}

TEST_F(EngineInputPropertyManagerTest, DefaultValueFromEngineValue)
{
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };

    auto manager =
        GetObjectRegistry().Create<IEngineInputPropertyManager>(META_NS::ClassId::EngineInputPropertyManager);
    ASSERT_TRUE(manager);
    EXPECT_TRUE(manager->GetValueManager()->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 2));
    manager->GetValueManager()->Sync(EngineSyncDirection::FROM_ENGINE);

    auto p = manager->ConstructProperty<int32_t>("value");
    ASSERT_TRUE(p);
    EXPECT_EQ(p->GetValue(), 2);
}

} // namespace Test
META_END_NAMESPACE()
