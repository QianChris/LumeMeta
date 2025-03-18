/**
 *  Copyright (C) 2023 Huawei Technologies Co, Ltd.
 */
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <meta/api/object.h>
#include <meta/base/meta_types.h>
#include <meta/ext/any_builder.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object_fwd.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_ser_output.h>
#include <meta/interface/serialization/intf_serializable.h>

#include "serialisation_utils.h"
#include "testing_objects.h"
#include "util.h"

using namespace testing;

META_BEGIN_NAMESPACE()
namespace Test {

TEST(SerializationTest, ExportImportWeakPtrPropertyDefault)
{
    TestSerialiser ser;
    {
        auto cont = CreateTestContainer<IContainer>();
        auto obj = CreateTestType("MyTest");
        // put the object to container so that we are able to make uri to it (to find anchor object)
        cont->Add(obj);
        auto prop1 = ConstructProperty<ITestType::WeakPtr>("test", obj);
        auto prop2 = ConstructProperty<ITestType::Ptr>("object", obj);

        auto meta = interface_cast<IMetadata>(cont);
        meta->AddProperty(prop1);
        meta->AddProperty(prop2);

        ASSERT_TRUE(ser.Export(cont));
        ser.Dump("file://./test.json");
    }
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    auto p1 = obj->GetProperty<ITestType::Ptr>("object");
    auto p2 = obj->GetProperty<ITestType::WeakPtr>("test");
    ASSERT_TRUE(p1);
    ASSERT_TRUE(p2);
    ASSERT_TRUE(p2->GetValue().lock() == p1->GetValue());
}

TEST(SerializationTest, ExportImportWeakPtrPropertyValue)
{
    TestSerialiser ser;
    {
        auto cont = CreateTestContainer<IContainer>();
        auto obj = CreateTestType("MyTest");
        // put the object to container so that we are able to make uri to it (to find anchor object)
        cont->Add(obj);
        auto prop1 = ConstructProperty<ITestType::WeakPtr>("test");
        auto prop2 = ConstructProperty<ITestType::Ptr>("object", obj);

        prop1->SetValue(obj);

        auto meta = interface_cast<IMetadata>(cont);
        meta->AddProperty(prop1);
        meta->AddProperty(prop2);

        ASSERT_TRUE(ser.Export(cont));
        ser.Dump("file://./test.json");
    }
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    auto p1 = obj->GetProperty<ITestType::Ptr>("object");
    auto p2 = obj->GetProperty<ITestType::WeakPtr>("test");
    ASSERT_TRUE(p1);
    ASSERT_TRUE(p2);
    ASSERT_TRUE(p2->GetValue().lock() == p1->GetValue());
}

TEST(SerializationTest, DISABLED_ExportImportWeakPtrPropertyWithGlobalAnchor)
{
    auto cont = CreateTestContainer<IContainer>();
    GetObjectRegistry().GetGlobalSerializationData().RegisterGlobalObject(interface_pointer_cast<IObject>(cont));
    TestSerialiser ser;
    {
        auto obj = CreateTestType("MyTest");
        // put the object to container so that we are able to make uri to it (to find anchor object)
        cont->Add(obj);
        auto prop1 = ConstructProperty<ITestType::WeakPtr>("test", obj);
        auto prop2 = ConstructProperty<ITestType::Ptr>("object", obj);

        Object object;
        object.AddProperty(prop1);
        object.AddProperty(prop2);

        ASSERT_TRUE(ser.Export(object));
        ser.Dump("file://./test.json");
    }
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    auto p1 = obj->GetProperty<ITestType::Ptr>("object");
    auto p2 = obj->GetProperty<ITestType::WeakPtr>("test");
    ASSERT_TRUE(p1);
    ASSERT_TRUE(p2);
    // They point to different objects because the weak ptr is serialised as ref uri, when as the ptr as object
    EXPECT_EQ(p2->GetValue().lock(), p1->GetValue());

    GetObjectRegistry().GetGlobalSerializationData().UnregisterGlobalObject(interface_pointer_cast<IObject>(cont));
}

TEST(SerializationTest, PropertyBind)
{
    TestSerialiser ser;
    {
        Object obj;
        auto p = ConstructProperty<uint32_t>("test");
        auto source = ConstructProperty<uint32_t>("source");
        source->SetValue(2);
        EXPECT_TRUE(p->SetBind(source));
        obj.AddProperty(p);
        obj.AddProperty(source);
        ASSERT_TRUE(ser.Export(obj));
        ser.Dump("file://./test.json");
    }
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);

    auto p1 = obj->GetProperty<uint32_t>("test");
    auto p2 = obj->GetProperty<uint32_t>("source");
    ASSERT_TRUE(p1);
    ASSERT_TRUE(p2);
    EXPECT_EQ(p1->GetValue(), 2);
    EXPECT_EQ(p2->GetValue(), 2);
    p2->SetValue(3);
    EXPECT_EQ(p1->GetValue(), 3);
}

TEST(SerializationTest, SerialiseBind)
{
    // serialise bind that requires resolving the bind later on when the target property owner has been serialised
    TestSerialiser ser;
    {
        Object target;
        auto p1 = ConstructProperty<int>("p1");
        target.AddProperty(p1);

        Object obj;
        auto p2 = ConstructProperty<int>("p2");
        obj.AddProperty(p2);
        obj.Attach(target);

        ASSERT_TRUE(p2->SetBind(p1));

        p1->SetValue(2);
        EXPECT_EQ(p2->GetValue(), 2);

        ASSERT_TRUE(ser.Export(obj));
        ser.Dump("file://./bind_ser_test.json");
    }

    auto nobj = ser.Import<IMetadata>();
    ASSERT_TRUE(nobj);
    auto att = interface_cast<IAttach>(nobj);
    ASSERT_TRUE(att);

    auto p2 = nobj->GetProperty<int>("p2");
    auto all = att->GetAttachments<IMetadata>();
    ASSERT_EQ(all.size(), 1);
    auto target = interface_cast<IMetadata>(all[0]);
    auto p1 = target->GetProperty<int>("p1");

    EXPECT_EQ(p2->GetValue(), 2);
    p1->SetValue(3);
    EXPECT_EQ(p2->GetValue(), 3);
}

} // namespace Test
META_END_NAMESPACE()
