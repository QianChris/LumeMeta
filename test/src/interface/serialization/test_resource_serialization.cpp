/**
 *  Copyright (C) 2023 Huawei Technologies Co, Ltd.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <core/resources/intf_resource_manager.h>

#include <meta/api/object.h>
#include <meta/base/meta_types.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/resource/intf_resource.h>

#include "serialisation_utils.h"

using namespace testing;

META_BEGIN_NAMESPACE()
namespace Test {

class ResourceSerializationTest : public ::testing::Test {
public:
protected:
    void SetUp() override
    {
        resources_ = GetObjectRegistry().Create<CORE_NS::IResourceManager>(ClassId::FileResourceManager);
        ASSERT_TRUE(resources_);

        ASSERT_TRUE(resources_->AddResourceType(
            GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::ObjectResourceType)));
        auto res = GetObjectRegistry().Create<CORE_NS::IResource>(ClassId::ObjectResource);
        if (auto m = interface_cast<IMetadata>(res)) {
            m->AddProperty(ConstructProperty<int>("prop", 1));
        }
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(res)) {
            i->SetResourceId("file://test_resource.json");
        }
        ASSERT_TRUE(resources_->AddResource(res));
    }

    void TearDown() override {}

protected:
    CORE_NS::IResourceManager::Ptr resources_;
};

TEST_F(ResourceSerializationTest, Basic)
{
    TestSerialiser ser;
    ser.SetResourceManager(resources_);
    {
        Object obj;
        auto prop = ConstructProperty<IObject::ConstPtr>(
            "test", interface_pointer_cast<IObject>(resources_->GetResource("file://test_resource.json")));
        ASSERT_TRUE(prop);
        ASSERT_TRUE(prop->GetValue());
        obj.AddProperty(prop);
        ser.Export(obj);
    }
    ser.Dump("file://./resource.json");
    auto m = ser.Import<IMetadata>();
    ASSERT_TRUE(m);
    auto p = m->GetProperty<IObject::ConstPtr>("test");
    ASSERT_TRUE(p);
    auto res = interface_pointer_cast<CORE_NS::IResource>(p->GetValue());
    ASSERT_TRUE(res);
    EXPECT_EQ(res, resources_->GetResource("file://test_resource.json"));
    EXPECT_EQ(res->GetResourceId(), "file://test_resource.json");
    auto resm = interface_cast<IMetadata>(res);
    ASSERT_TRUE(resm);
    auto rp = resm->GetProperty<int>("prop");
    ASSERT_TRUE(rp);
    EXPECT_EQ(rp->GetValue(), 1);
}

TEST_F(ResourceSerializationTest, ResourceManagerSerialisation)
{
    ASSERT_EQ(resources_->Export("file://test.resources"), CORE_NS::IResourceManager::Status::OK);

    auto resman = GetObjectRegistry().Create<CORE_NS::IResourceManager>(ClassId::FileResourceManager);
    ASSERT_TRUE(resman);
    ASSERT_TRUE(
        resman->AddResourceType(GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::ObjectResourceType)));

    ASSERT_EQ(resman->Import("file://test.resources"), CORE_NS::IResourceManager::Status::OK);

    auto res = resman->GetResource("file://test_resource.json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->GetResourceId(), "file://test_resource.json");
    auto resm = interface_cast<IMetadata>(res);
    ASSERT_TRUE(resm);
    auto rp = resm->GetProperty<int>("prop");
    ASSERT_TRUE(rp);
    EXPECT_EQ(rp->GetValue(), 1);
}

} // namespace Test
META_END_NAMESPACE()
