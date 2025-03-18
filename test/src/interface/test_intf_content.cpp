/**
 *  Copyright (C) 2023 Huawei Technologies Co, Ltd.
 */

#include <tester.h>

#include <gtest/gtest.h>

#include <meta/api/content_object.h>
#include <meta/api/iteration.h>
#include <meta/api/loaders/json_content_loader.h>
#include <meta/interface/loaders/intf_class_content_loader.h>

#include "serialisation_utils.h"
#include "testing_objects.h"
#include "util.h"

META_BEGIN_NAMESPACE()
namespace Test {
using namespace testing;

TEST(ContentTest, Search)
{
    auto content = CreateTestType<IObject>("Content");

    ContentObject co;
    co.SetContent(content);
    auto c = CreateTestContainer<IContainer>();
    c->Add(co);
    EXPECT_FALSE(c->FindAny({ "Content", TraversalType::NO_HIERARCHY }));
    EXPECT_EQ(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }), content);
    EXPECT_EQ(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestType::UID } }), content);
    EXPECT_EQ(c->FindAny({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestType::UID } }), content);
    EXPECT_FALSE(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER, { IContainer::UID } }));

    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 2);
    EXPECT_EQ(c->FindAll({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 1);

    EXPECT_TRUE(ContainsObjectWithName(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }), "Content"));

    co.ContentSearchable(false);
    EXPECT_FALSE(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }));
    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 1);

    co.ContentSearchable(true);
    EXPECT_EQ(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }), content);
    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 2);
}

TEST(ContentTest, SearchContentContainer)
{
    auto ccontent = CreateTestType<IObject>("Content");
    auto content = CreateTestContainer<IContainer>();
    content->Add(ccontent);

    ContentObject co;
    co.SetContent(interface_pointer_cast<IObject>(content));
    auto c = CreateTestContainer<IContainer>();
    c->Add(co);
    EXPECT_FALSE(c->FindAny({ "Content", TraversalType::NO_HIERARCHY }));
    EXPECT_EQ(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }), ccontent);
    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 3);

    co.ContentSearchable(false);
    EXPECT_FALSE(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }));
    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 1);

    co.ContentSearchable(true);
    EXPECT_EQ(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }), ccontent);
    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 3);
}

TEST(ContentTest, Iteration)
{
    auto ccontent = CreateTestType<IObject>("Content");
    auto content = CreateTestContainer<IContainer>();
    content->Add(ccontent);

    ContentObject co;
    co.SetContent(interface_pointer_cast<IObject>(content));
    auto c = CreateTestContainer<IContainer>();
    c->Add(co);

    {
        BASE_NS::vector<IObject::Ptr> vec;
        EXPECT_TRUE(ForEachShared(
            c, [&](const IObject::Ptr& obj) { vec.push_back(obj); }, TraversalType::DEPTH_FIRST_PRE_ORDER));
        EXPECT_EQ(vec.size(), 3);
        EXPECT_TRUE(ContainsObjectWithName(vec, "Content"));
    }

    co.ContentSearchable(false);

    {
        BASE_NS::vector<IObject::Ptr> vec;
        EXPECT_TRUE(ForEachShared(
            c, [&](const IObject::Ptr& obj) { vec.push_back(obj); }, TraversalType::DEPTH_FIRST_PRE_ORDER));
        EXPECT_EQ(vec.size(), 1);
        EXPECT_FALSE(ContainsObjectWithName(vec, "Content"));
    }
}

TEST(ContentTest, Serialization)
{
    {
        TestSerialiser ser;

        ContentObject co;
        co.SerializeContent(false);
        co.SetContent(CreateTestType<IObject>("Content"));
        ASSERT_TRUE(ser.Export(co));

        auto imp = ser.Import<IContent>();
        ASSERT_TRUE(imp);
        EXPECT_FALSE(imp->Content()->GetValue());
    }
    {
        TestSerialiser ser;

        ContentObject co;
        co.SerializeContent(true);
        co.SetContent(CreateTestType<IObject>("Content"));
        ASSERT_TRUE(ser.Export(co));

        ser.Dump("file://./content_test.json");

        auto imp = ser.Import<IContent>();
        ASSERT_TRUE(imp);
        ASSERT_TRUE(imp->Content()->GetValue());
        EXPECT_EQ(imp->Content()->GetValue()->GetName(), "Content");
    }
}

TEST(ContentTest, Defaults)
{
    ContentObject co;
    const auto objectFlags = interface_cast<IObjectFlags>(co);
    ASSERT_TRUE(objectFlags);
    EXPECT_FALSE(objectFlags->GetObjectFlags().IsSet(META_NS::ObjectFlagBits::SERIALIZE_HIERARCHY));
}

TEST(ContentTest, ObjectFlagsNoHierarchy)
{
    TestSerialiser ser;
    ContentObject co;
    co.SetContent(CreateTestType<IObject>("Content"));

    // By default ContentObject should not export its Content (i.e. ObjectFlagBits::SERIALIZE_HIERARCHY is not set)
    ASSERT_TRUE(ser.Export(co));

    auto imported = ser.Import<IContent>();
    ASSERT_TRUE(imported);

    EXPECT_FALSE(imported->Content()->GetValue());
}

TEST(ContentTest, ObjectFlagsHierarchy)
{
    TestSerialiser ser;
    ContentObject co;
    co.SetContent(CreateTestType<IObject>("Content"));

    auto flagsIntfPtr = interface_cast<IObjectFlags>(co);
    ASSERT_TRUE(flagsIntfPtr);
    SetObjectFlags(co, ObjectFlagBits::SERIALIZE_HIERARCHY, true);

    ASSERT_TRUE(ser.Export(co));
    auto imported = ser.Import<IContent>();
    ASSERT_TRUE(imported);

    auto content = GetValue(imported->Content());
    ASSERT_TRUE(content);
    EXPECT_EQ(content->GetName(), "Content");
}

TEST(ContentTest, ContentRequiredInterfaces)
{
    ContentObject co;
    EXPECT_TRUE(co.SetRequiredInterfaces({ ITestType::UID }));

    auto correctType = GetObjectRegistry().Create(ClassId::TestType);
    auto wrongType = GetObjectRegistry().Create(ClassId::Object);

    EXPECT_FALSE(co.SetContent(wrongType));
    EXPECT_FALSE(co.Content()->GetValue());

    EXPECT_TRUE(co.SetContent(correctType));
    EXPECT_EQ(co.Content()->GetValue(), correctType);

    // Relax requirements
    EXPECT_TRUE(co.SetRequiredInterfaces({}));
    EXPECT_EQ(co.Content()->GetValue(), correctType);

    EXPECT_TRUE(co.SetContent(wrongType));
    EXPECT_EQ(co.Content()->GetValue(), wrongType);
}

} // namespace Test
META_END_NAMESPACE()
