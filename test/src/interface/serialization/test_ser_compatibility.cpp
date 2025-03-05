/**
 *  Copyright (C) 2023 Huawei Technologies Co, Ltd.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <meta/api/object.h>
#include <meta/ext/object_fwd.h>

#include "serialisation_utils.h"
#include "test_data_helpers.h"
#include "test_utils.h"
#include "util.h"

using namespace testing;

META_BEGIN_NAMESPACE()
namespace Test {

TEST(SerializationCompatTest, TestType)
{
    constexpr auto path = "file://./assets/metav1/test_type.json";
    TestSerialiser ser;
    ASSERT_TRUE(ser.LoadFile(path));
    auto obj = ser.Import<ITestType>();
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->First()->GetValue(), 2);
    EXPECT_EQ(obj->Second()->GetValue(), "Hipshops");
    // old system seems to export array property value even though it is just default,
    // so this makes things not equal
    // EXPECT_TRUE(IsEqual(interface_pointer_cast<IMetadata>(CreateTestType()), obj));
}

TEST(SerializationCompatTest, TestTypeWithAttachment)
{
    constexpr auto path = "file://./assets/metav1/test_type_with_attachment.json";
    TestSerialiser ser;
    ASSERT_TRUE(ser.LoadFile(path));
    auto obj = ser.Import<ITestType>();
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->First()->GetValue(), 2);
    EXPECT_EQ(obj->Second()->GetValue(), "Hipshops");

    auto att = interface_cast<IAttach>(obj);
    ASSERT_TRUE(att);
    auto cont = att->GetAttachments();
    IMetadata::Ptr attobj;
    for (auto&& v : cont) {
        if (!v->GetInterface(IProperty::UID)) {
            attobj = interface_pointer_cast<IMetadata>(v);
        }
    }

    ASSERT_TRUE(attobj);
    auto p = attobj->GetProperty<BASE_NS::string>("Test");
    EXPECT_EQ(p->GetValue(), "Some");
}

TEST(SerializationCompatTest, Bind)
{
    constexpr auto path = "file://./assets/metav1/bind.json";
    TestSerialiser ser;
    ASSERT_TRUE(ser.LoadFile(path));
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);

    auto p1 = obj->GetProperty<int>("p1");
    ASSERT_TRUE(p1);
    EXPECT_EQ(p1->GetValue(), 3);
    auto p2 = obj->GetProperty<int>("p2");
    ASSERT_TRUE(p2);
    EXPECT_EQ(p2->GetValue(), 3);

    p1->SetValue(2);
    EXPECT_EQ(p1->GetValue(), 2);
    EXPECT_EQ(p2->GetValue(), 2);
}

TEST(SerializationCompatTest, Obj1)
{
    constexpr auto path = "file://./assets/metav1/obj1.json";
    TestSerialiser ser;
    ASSERT_TRUE(ser.LoadFile(path));
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);

    auto p = obj->GetProperty<IObject::Ptr>("Test");
    ASSERT_TRUE(p);
    EXPECT_FALSE(p->GetValue());
};

TEST(SerializationCompatTest, Obj2)
{
    constexpr auto path = "file://./assets/metav1/obj2.json";
    TestSerialiser ser;
    ASSERT_TRUE(ser.LoadFile(path));
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);

    auto p = obj->GetProperty<IObject::Ptr>("Test");
    ASSERT_TRUE(p);
    EXPECT_TRUE(p->GetValue());
};

TEST(SerializationCompatTest, LocalStringProperties)
{
    constexpr auto path = "file://./assets/metav1/local_strings.json";
    TestSerialiser ser;
    ASSERT_TRUE(ser.LoadFile(path));
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    auto strings = obj->GetProperty<IObject::Ptr>("strings");
    ASSERT_TRUE(strings);
    auto value = interface_cast<IMetadata>(strings->GetValue());
    ASSERT_TRUE(value);
    {
        auto lang = value->GetProperty<IObject::Ptr>("en");
        ASSERT_TRUE(lang);
        auto obj = interface_cast<IMetadata>(lang->GetValue());
        ASSERT_TRUE(obj);
        auto p = obj->GetProperty<BASE_NS::string>("Text1");
        ASSERT_TRUE(p);
        EXPECT_EQ(p->GetValue(), "Text");
    }
    {
        auto lang = value->GetProperty<IObject::Ptr>("fi");
        ASSERT_TRUE(lang);
        auto obj = interface_cast<IMetadata>(lang->GetValue());
        ASSERT_TRUE(obj);
        auto p = obj->GetProperty<BASE_NS::string>("Text1");
        ASSERT_TRUE(p);
        EXPECT_EQ(p->GetValue(), "Teksti");
    }
}

// TEST(SerializationTest, GenerateCompat)
// {
//     {
//         TestSerialiser ser;
//         auto object = CreateTestType();
//         object->First()->Set(2);
//         object->Second()->Set("Hipshops");
//         ASSERT_TRUE(ser.Export(object));
//         ser.Dump("file://./test_type.json");
//     }

//     {
//         TestSerialiser ser;
//         auto object = CreateTestType();
//         object->First()->Set(2);
//         object->Second()->Set("Hipshops");
//         Object obj;
//         obj.AddProperty(ConstructProperty<BASE_NS::string>(GetObjectRegistry(), "Test", "Some"));

//         auto att = interface_cast<IAttach>(object);
//         ASSERT_TRUE(att);
//         att->Attach(obj);

//         ASSERT_TRUE(ser.Export(object));
//         ser.Dump("file://./test_type_with_attachment.json");
//     }

//     {
//         TestSerialiser ser;
//         Object object;
//         object.AddProperty(ConstructProperty<IObject::Ptr>(GetObjectRegistry(), "Test", nullptr));
//         ASSERT_TRUE(ser.Export(object));
//         ser.Dump("file://./obj1.json");
//     }

//     {
//         TestSerialiser ser;
//         Object object1;
//         Object object2;
//         auto p = ConstructProperty<IObject::Ptr>(GetObjectRegistry(), "Test", nullptr);
//         p->Set(object2);
//         object1.AddProperty(p);
//         ASSERT_TRUE(ser.Export(object1));
//         ser.Dump("file://./obj2.json");
//     }

//     {
//         TestSerialiser ser;
//         Object object;
//         auto p1 = ConstructProperty<int>(GetObjectRegistry(), "p1", 0);
//         auto p2 = ConstructProperty<int>(GetObjectRegistry(), "p2", 0);
//         p2->Bind(p1);
//         p1->Set(3);
//         object.AddProperty(p1);
//         object.AddProperty(p2);
//         ASSERT_TRUE(ser.Export(object));
//         ser.Dump("file://./bind.json");
//     }
// }

} // namespace Test
META_END_NAMESPACE()
