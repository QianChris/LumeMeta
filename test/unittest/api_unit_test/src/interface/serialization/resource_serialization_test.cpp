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

#include <core/resources/intf_resource_manager.h>

#include <meta/api/object.h>
#include <meta/base/meta_types.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/resource/intf_resource.h>

#include "helpers/serialisation_utils.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

META_BEGIN_NAMESPACE()
namespace UTest {

class API_ResourceSerializationTest : public ::testing::Test {
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
            i->SetResourceId("app://test_resource.json");
        }
        ASSERT_TRUE(resources_->AddResource(res));

        resources_->SetFileManager(CORE_NS::IFileManager::Ptr(&GetTestEnv()->engine->GetFileManager()));
    }

    void TearDown() override {}

protected:
    CORE_NS::IResourceManager::Ptr resources_;
};

UNIT_TEST_F(API_ResourceSerializationTest, Basic, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    ser.SetResourceManager(resources_);
    {
        Object obj(CreateObjectInstance(ClassId::Object));
        auto prop = ConstructProperty<IObject::ConstPtr>(
            "test", interface_pointer_cast<IObject>(resources_->GetResource("app://test_resource.json")));
        ASSERT_TRUE(prop);
        ASSERT_TRUE(prop->GetValue());
        Metadata(obj).AddProperty(prop);
        ser.Export(obj);
    }
    ser.Dump("app://resource.json");
    auto m = ser.Import<IMetadata>();
    ASSERT_TRUE(m);
    auto p = m->GetProperty<IObject::ConstPtr>("test");
    ASSERT_TRUE(p);
    auto res = interface_pointer_cast<CORE_NS::IResource>(p->GetValue());
    ASSERT_TRUE(res);
    EXPECT_EQ(res, resources_->GetResource("app://test_resource.json"));
    EXPECT_EQ(res->GetResourceId(), "app://test_resource.json");
    auto resm = interface_cast<IMetadata>(res);
    ASSERT_TRUE(resm);
    auto rp = resm->GetProperty<int>("prop");
    ASSERT_TRUE(rp);
    EXPECT_EQ(rp->GetValue(), 1);
}

UNIT_TEST_F(API_ResourceSerializationTest, ResourceManagerSerialisation, testing::ext::TestSize.Level1)
{
    ASSERT_EQ(resources_->Export("app://test.resources"), CORE_NS::IResourceManager::Result::OK);

    auto resman = GetObjectRegistry().Create<CORE_NS::IResourceManager>(ClassId::FileResourceManager);
    ASSERT_TRUE(resman);
    ASSERT_TRUE(
        resman->AddResourceType(GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::ObjectResourceType)));

    resman->SetFileManager(CORE_NS::IFileManager::Ptr(&GetTestEnv()->engine->GetFileManager()));
    ASSERT_EQ(resman->Import("app://test.resources"), CORE_NS::IResourceManager::Result::OK);

    auto res = resman->GetResource("app://test_resource.json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->GetResourceId(), "app://test_resource.json");
    auto resm = interface_cast<IMetadata>(res);
    ASSERT_TRUE(resm);
    auto rp = resm->GetProperty<int>("prop");
    ASSERT_TRUE(rp);
    EXPECT_EQ(rp->GetValue(), 1);
}

} // namespace UTest
META_END_NAMESPACE()
