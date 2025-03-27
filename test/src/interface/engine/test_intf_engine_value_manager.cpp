/**
 *  Copyright (C) 2023 Huawei Technologies Co, Ltd.
 */
// clang-format off
#include "engine_test_property.h"
// clang-format on

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <core/property/property_handle_util.h>

#include <meta/api/engine/util.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/engine/intf_engine_value_manager.h>
#include <meta/interface/intf_object_registry.h>

#include "tester.h"
#include "testing_objects.h"
#include "util.h"

using namespace testing;

META_BEGIN_NAMESPACE()
namespace Test {

class EngineManagerTest : public ::testing::Test {
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

TEST_F(EngineManagerTest, Values)
{
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    EXPECT_TRUE(manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));
    EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_PROPERTY_COUNT);
    auto p = manager->ConstructProperty<int32_t>("value");
    ASSERT_TRUE(p);
    EXPECT_EQ(p->GetValue(), 0);

    auto info = interface_cast<IInfo>(&p->GetValueAny());
    ASSERT_TRUE(info);
    EXPECT_EQ(info->GetName(), "value");
    EXPECT_EQ(info->GetDescription(), "Value it is");

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 1));
    EXPECT_EQ(p->GetValue(), 0);
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(p->GetValue(), 1);

    EXPECT_TRUE(p->SetValue(2));
    EXPECT_EQ(p->GetValue(), 2);
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));

    EXPECT_EQ(p->GetValue(), 2);
    EXPECT_EQ(CORE_NS::GetPropertyValue<int32_t>(*cman.GetData(0), "value"), 2);

    // update
    {
        EXPECT_TRUE(manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));
        EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_PROPERTY_COUNT);
        auto p = manager->ConstructProperty<int32_t>("value");
        ASSERT_TRUE(p);
    }

    {
        manager->RemoveValue("value");
        EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_PROPERTY_COUNT - 1);
        EXPECT_TRUE(manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));
        EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_PROPERTY_COUNT);
        auto p = manager->ConstructProperty<int32_t>("value");
        ASSERT_TRUE(p);
    }

    {
        manager->RemoveAll();
        EXPECT_EQ(manager->GetAllEngineValues().size(), 0);
        EXPECT_TRUE(manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));
        EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_PROPERTY_COUNT);
        auto p = manager->ConstructProperty<int32_t>("value");
        ASSERT_TRUE(p);
    }
}

TEST_F(EngineManagerTest, ArrayValues)
{
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    EXPECT_TRUE(manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));
    auto p = manager->ConstructArrayProperty<float>("floats");
    ASSERT_TRUE(p);
    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<float> {}));

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "floats", (BASE_NS::vector<float> { 1, 2 })));
    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<float> {}));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<float> { 1, 2 }));

    EXPECT_TRUE(p->SetValue((BASE_NS::vector<float> { 3, 4, 5 })));
    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<float> { 3, 4, 5 }));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));

    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<float> { 3, 4, 5 }));
    EXPECT_EQ(CORE_NS::GetPropertyValue<BASE_NS::vector<float>>(*cman.GetData(0), "floats"),
        (BASE_NS::vector<float> { 3, 4, 5 }));
}

TEST_F(EngineManagerTest, AddValuesRecursively)
{
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    AddEngineValuesRecursively(manager, EnginePropertyHandle { &cman, cman.entityRef });
    EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_RECURSIVE_PROPERTY_COUNT);
    auto p = manager->ConstructProperty<BASE_NS::Math::IVec2>("vec2");
    ASSERT_TRUE(p);
    auto x = manager->ConstructProperty<int32_t>("vec2.x");
    ASSERT_TRUE(x);
    auto y = manager->ConstructProperty<int32_t>("vec2.y");
    ASSERT_TRUE(y);

    EXPECT_EQ(p->GetValue(), BASE_NS::Math::IVec2());
    EXPECT_EQ(x->GetValue(), 0);
    EXPECT_EQ(y->GetValue(), 0);

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "vec2", BASE_NS::Math::IVec2(1, 2)));
    EXPECT_EQ(p->GetValue(), BASE_NS::Math::IVec2(0, 0));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(p->GetValue(), BASE_NS::Math::IVec2(1, 2));
    EXPECT_EQ(x->GetValue(), 1);
    EXPECT_EQ(y->GetValue(), 2);

    EXPECT_TRUE(p->SetValue(BASE_NS::Math::IVec2(2, 3)));
    EXPECT_EQ(p->GetValue(), BASE_NS::Math::IVec2(2, 3));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));

    EXPECT_EQ(p->GetValue(), BASE_NS::Math::IVec2(2, 3));
    // call again to ensure the x and y are updated from engine, as the update depends on the order of values
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(x->GetValue(), 2);
    EXPECT_EQ(y->GetValue(), 3);
    EXPECT_EQ(CORE_NS::GetPropertyValue<BASE_NS::Math::IVec2>(*cman.GetData(0), "vec2"), BASE_NS::Math::IVec2(2, 3));

    EXPECT_TRUE(x->SetValue(11));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(x->GetValue(), 11);
    // call again to ensure the x and y are updated from engine, as the update depends on the order of values
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(p->GetValue(), BASE_NS::Math::IVec2(11, 3));

    auto typex = manager->ConstructProperty<int32_t>("type.vec.x");
    ASSERT_TRUE(typex);
    EXPECT_TRUE(typex->SetValue(9));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    auto type = manager->ConstructProperty<BASE_NS::Math::IVec2>("type.vec");
    ASSERT_TRUE(type);
    EXPECT_EQ(type->GetValue(), BASE_NS::Math::IVec2(9, 0));
}

TEST_F(EngineManagerTest, AddValuesRecursivelyWithHandle)
{
    TestComponentManager<prop1::EngineTestProp> cman1 { prop1::ENGINE_TESTPROP_METADATA };
    TestComponentManager<prop2::EngineTestProp> cman2 { prop2::ENGINE_TESTPROP_METADATA };

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman1.GetData(0), "handle", cman2.GetData(0)));

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    AddEngineValuesRecursively(manager, EnginePropertyHandle { &cman1, cman1.entityRef });
    EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_RECURSIVE_PROPERTY_COUNT + prop2::TEST_PROPERTY_COUNT);

    auto handle = manager->ConstructProperty<CORE_NS::IPropertyHandle*>("handle");
    ASSERT_TRUE(handle);
    EXPECT_EQ(handle->GetValue(), cman2.GetData(0));

    auto value = manager->ConstructProperty<int>("handle.value");
    ASSERT_TRUE(value);
    EXPECT_EQ(value->GetValue(), 1);

    auto array = manager->ConstructProperty<BASE_NS::vector<uint32_t>>("handle.array");
    ASSERT_TRUE(array);
    EXPECT_EQ(array->GetValue(), (BASE_NS::vector<uint32_t> { 1, 1 }));
}

TEST_F(EngineManagerTest, ChangeHandle)
{
    TestComponentManager<prop1::EngineTestProp> cman1 { prop1::ENGINE_TESTPROP_METADATA };
    TestComponentManager<prop2::EngineTestProp> cman2 { prop2::ENGINE_TESTPROP_METADATA };
    TestComponentManager<prop2::EngineTestProp> cman3 { prop2::ENGINE_TESTPROP_METADATA };

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman1.GetData(0), "handle", cman2.GetData(0)));

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    //    manager->ConstructValue(EnginePropertyHandle {&cman1, cman1.entityRef}, "handle.value", EngineValueOptions{});
    //    EXPECT_EQ(manager->GetAllEngineValues().size(), 2);
    AddEngineValuesRecursively(manager, EnginePropertyHandle { &cman1, cman1.entityRef });
    EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_RECURSIVE_PROPERTY_COUNT + prop2::TEST_PROPERTY_COUNT);

    auto handle = manager->ConstructProperty<CORE_NS::IPropertyHandle*>("handle");
    ASSERT_TRUE(handle);
    EXPECT_EQ(handle->GetValue(), cman2.GetData(0));

    auto value = manager->ConstructProperty<int>("handle.value");
    ASSERT_TRUE(value);
    EXPECT_EQ(value->GetValue(), 1);

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman3.GetData(0), "value", 9));
    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman1.GetData(0), "handle", cman3.GetData(0)));

    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));

    EXPECT_EQ(value->GetValue(), 9);
    EXPECT_EQ(handle->GetValue(), cman3.GetData(0));
}

template<typename Type>
void TestWithSingle(BASE_NS::string_view path, EngineValueOptions ops = {})
{
    TestComponentManager<prop1::EngineTestProp> cman1 { prop1::ENGINE_TESTPROP_METADATA };
    TestComponentManager<prop2::EngineTestProp> cman2 { prop2::ENGINE_TESTPROP_METADATA };

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman1.GetData(0), "handle", cman2.GetData(0)));

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    manager->ConstructValue(EnginePropertyHandle { &cman1, cman1.entityRef }, path, ops);
    EXPECT_TRUE(manager->GetAllEngineValues().size() >= 1);

    auto name = ops.namePrefix;
    if (!name.empty()) {
        name += ".";
    }
    name += path;

    EXPECT_TRUE(manager->ConstructProperty<Type>(name)) << "Property: " << name;
}

TEST_F(EngineManagerTest, AddSingleValue)
{
    TestWithSingle<int>("value");
    TestWithSingle<CORE_NS::IPropertyHandle*>("handle");
    TestWithSingle<int>("handle.value");
    TestWithSingle<BASE_NS::vector<uint32_t>>("handle.array");
    TestWithSingle<BASE_NS::vector<EngineTestType>>("handle.complexVec");

    TestWithSingle<uint32_t>("handle.array[0]");
    TestWithSingle<uint32_t>("handle.array[1]");
    TestWithSingle<BASE_NS::vector<EngineTestType>>("handle.complex");
    TestWithSingle<EngineTestType>("handle.complex[1]");
    TestWithSingle<BASE_NS::Math::IVec2>("handle.complex[1].vec");
    TestWithSingle<int32_t>("handle.complex[2].vec.x");
    TestWithSingle<int32_t>("handle.complex[2].vec.y");
    TestWithSingle<int32_t>("handle.complexVec[0].vec.x");
    TestWithSingle<int32_t>("handle.complexVec[1].vec.y");

    TestWithSingle<BASE_NS::Math::IVec2>("type.vec");
    TestWithSingle<int32_t>("type.vec.x");

    TestWithSingle<int32_t>("type.vec.x", EngineValueOptions { "TestPrefix" });
}

// check the overlapping values don't overwrite each other if not altered
TEST_F(EngineManagerTest, Overlapping)
{
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    AddEngineValuesRecursively(manager, EnginePropertyHandle { &cman, cman.entityRef });
    EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_RECURSIVE_PROPERTY_COUNT);

    auto p = manager->ConstructProperty<BASE_NS::Math::IVec2>("vec2");
    ASSERT_TRUE(p);
    auto x = manager->ConstructProperty<int32_t>("vec2.x");
    ASSERT_TRUE(x);

    x->SetValue(2);
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::TO_ENGINE));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::FROM_ENGINE));
    EXPECT_EQ(p->GetValue(), (BASE_NS::Math::IVec2 { 2, 0 }));
    EXPECT_EQ(x->GetValue(), 2);

    p->SetValue({ 3, 0 });
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::TO_ENGINE));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::FROM_ENGINE));
    EXPECT_EQ(p->GetValue(), (BASE_NS::Math::IVec2 { 3, 0 }));
    EXPECT_EQ(x->GetValue(), 3);
}

TEST_F(EngineManagerTest, Enum)
{
    TestComponentManager<prop3::EngineTestProp> cman { prop3::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef });
    EXPECT_EQ(manager->GetAllEngineValues().size(), prop3::TEST_PROPERTY_COUNT);

    auto p = manager->ConstructProperty<EngineTestEnum>("enum1");
    ASSERT_TRUE(p);

    auto v = p->GetValueAny().Clone();
    ASSERT_TRUE(interface_cast<IEnum>(&p->GetValueAny()));
    auto ei = interface_cast<IEnum>(v);
    ASSERT_TRUE(ei);
    ASSERT_TRUE(interface_cast<IInfo>(v));
    auto et = interface_cast<IEngineType>(v);
    ASSERT_TRUE(et);

    EXPECT_EQ(et->GetTypeDecl(), MetaType<EngineTestEnum>::coreType);
    EXPECT_EQ(ei->GetName(), "enum1");
    EXPECT_EQ(ei->GetDescription(), "My Enum");
    EXPECT_EQ(ei->GetEnumType(), EnumType::SINGLE_VALUE);
    auto view = ei->GetValues();
    ASSERT_EQ(view.size(), 3);
    EXPECT_EQ(view[0].name, "MY_VALUE_1");
    EXPECT_EQ(view[0].desc, "Value 1");
    EXPECT_EQ(view[1].name, "MY_VALUE_2");
    EXPECT_EQ(view[1].desc, "Value 2");
    EXPECT_EQ(view[2].name, "MY_VALUE_3");
    EXPECT_EQ(view[2].desc, "Value 3");

    EXPECT_EQ(ei->GetValueIndex(), 0);
    EXPECT_TRUE(ei->IsValueSet(0));

    EXPECT_TRUE(ei->SetValueIndex(1));
    EXPECT_EQ(ei->GetValueIndex(), 1);
    EXPECT_TRUE(ei->IsValueSet(1));
    EXPECT_EQ(GetValue<EngineTestEnum>(*v), EngineTestEnum::MY_VALUE_2);

    EXPECT_TRUE(ei->SetValueIndex(2));
    EXPECT_EQ(ei->GetValueIndex(), 2);
    EXPECT_EQ(GetValue<EngineTestEnum>(*v), EngineTestEnum::MY_VALUE_3);
    EXPECT_EQ(GetValue<int64_t>(*v), 5);

    EXPECT_FALSE(ei->SetValueIndex(3));
}

TEST_F(EngineManagerTest, BitField)
{
    TestComponentManager<prop3::EngineTestProp> cman { prop3::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef });
    EXPECT_EQ(manager->GetAllEngineValues().size(), prop3::TEST_PROPERTY_COUNT);

    auto p = manager->ConstructProperty<EngineTestBitField>("enum2");
    ASSERT_TRUE(p);

    auto v = p->GetValueAny().Clone();
    ASSERT_TRUE(interface_cast<IEnum>(&p->GetValueAny()));
    auto ei = interface_cast<IEnum>(v);
    ASSERT_TRUE(ei);
    ASSERT_TRUE(interface_cast<IInfo>(v));
    auto et = interface_cast<IEngineType>(v);
    ASSERT_TRUE(et);

    EXPECT_EQ(et->GetTypeDecl(), MetaType<EngineTestBitField>::coreType);
    EXPECT_EQ(ei->GetName(), "enum2");
    EXPECT_EQ(ei->GetDescription(), "My Bitfield");
    EXPECT_EQ(ei->GetEnumType(), EnumType::BIT_FIELD);
    auto view = ei->GetValues();
    ASSERT_EQ(view.size(), 4);
    EXPECT_EQ(view[0].name, "MY_VALUE_1");
    EXPECT_EQ(view[0].desc, "Value 1");
    EXPECT_EQ(view[1].name, "MY_VALUE_2");
    EXPECT_EQ(view[1].desc, "Value 2");
    EXPECT_EQ(view[2].name, "MY_VALUE_3");
    EXPECT_EQ(view[2].desc, "Value 3");
    EXPECT_EQ(view[3].name, "MY_VALUE_4");
    EXPECT_EQ(view[3].desc, "Value 4");

    EXPECT_EQ(ei->GetValueIndex(), -1);
    EXPECT_FALSE(ei->IsValueSet(0));

    EXPECT_TRUE(ei->SetValueIndex(0));
    EXPECT_EQ(ei->GetValueIndex(), 0);
    EXPECT_TRUE(ei->IsValueSet(0));
    EXPECT_EQ(GetValue<EngineTestBitField>(*v), EngineTestBitField::MY_VALUE_1);

    EXPECT_TRUE(ei->FlipValue(1, true));
    EXPECT_TRUE(ei->IsValueSet(1));
    EXPECT_EQ(GetValue<int64_t>(*v), int64_t(EngineTestBitField::MY_VALUE_1) | int64_t(EngineTestBitField::MY_VALUE_2));

    EXPECT_TRUE(ei->FlipValue(0, false));
    EXPECT_FALSE(ei->IsValueSet(0));
    EXPECT_TRUE(ei->IsValueSet(1));
    EXPECT_EQ(GetValue<int64_t>(*v), int64_t(EngineTestBitField::MY_VALUE_2));
}

TEST_F(EngineManagerTest, ContainerSubscription)
{
    TestComponentManager<prop2::EngineTestProp> cman { prop2::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    bool arrres =
        manager->ConstructValue(EnginePropertyHandle { &cman, cman.entityRef }, "complexVec", EngineValueOptions {});
    ASSERT_TRUE(arrres);

    auto arr = manager->ConstructArrayProperty<EngineTestType>("complexVec");
    ASSERT_TRUE(arr);

    {
        bool res = manager->ConstructValue(
            EnginePropertyHandle { &cman, cman.entityRef }, "complexVec[2]", EngineValueOptions {});
        ASSERT_FALSE(res);
    }

    bool res0 =
        manager->ConstructValue(EnginePropertyHandle { &cman, cman.entityRef }, "complexVec[0]", EngineValueOptions {});
    ASSERT_TRUE(res0);

    auto v0 = manager->ConstructProperty<EngineTestType>("complexVec[0]");
    ASSERT_TRUE(v0);
    EXPECT_EQ(v0->GetValue(), (EngineTestType { { 1, 2 } }));

    bool res1 =
        manager->ConstructValue(EnginePropertyHandle { &cman, cman.entityRef }, "complexVec[1]", EngineValueOptions {});
    ASSERT_TRUE(res1);

    auto v1 = manager->ConstructProperty<EngineTestType>("complexVec[1]");
    ASSERT_TRUE(v1);
    EXPECT_EQ(v1->GetValue(), (EngineTestType { { 8, 8 } }));

    arr->SetValue({ EngineTestType { { 4, 4 } } });
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));

    EXPECT_EQ(v0->GetValue(), (EngineTestType { { 4, 4 } }));
    v0->SetValue({ { 5, 6 } });
    // old value is returned since there is nothing behind it any more
    EXPECT_EQ(v1->GetValue(), (EngineTestType { { 8, 8 } }));
    v1->SetValue({ { 3, 3 } });

    manager->Sync(EngineSyncDirection::AUTO);

    {
        bool res = manager->ConstructValue(
            EnginePropertyHandle { &cman, cman.entityRef }, "complexVec[0].vec.x", EngineValueOptions {});
        ASSERT_TRUE(res);

        auto v = manager->ConstructProperty<int32_t>("complexVec[0].vec.x");
        ASSERT_TRUE(v);
        EXPECT_EQ(v->GetValue(), 5);
        v->SetValue(7);
        manager->Sync(EngineSyncDirection::AUTO);
        EXPECT_EQ(v->GetValue(), 7);
    }
    EXPECT_EQ(v0->GetValue(), (EngineTestType { { 7, 6 } }));
}

} // namespace Test
META_END_NAMESPACE()
