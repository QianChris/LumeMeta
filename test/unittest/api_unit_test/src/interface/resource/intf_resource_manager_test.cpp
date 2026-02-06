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

#include <meta/interface/builtin_objects.h>
#include <meta/interface/resource/intf_object_resource.h>
#include <meta/interface/resource/intf_owned_resource_groups.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/resource/intf_resource_query.h>

#include "helpers/serialisation_utils.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

META_BEGIN_NAMESPACE()

namespace UTest {

META_REGISTER_CLASS(TestResource, "cfef223f-a9eb-4f19-a60e-b2a3238f5691", META_NS::ObjectCategoryBits::NO_CATEGORY)

class API_FileResourceManagerTest : public ::testing::Test {
public:
    void SetUp() override
    {
        resources = GetObjectRegistry().Create<CORE_NS::IResourceManager>(META_NS::ClassId::FileResourceManager);
        ASSERT_TRUE(resources);

        auto res = GetObjectRegistry().Create<IObjectResource>(META_NS::ClassId::ObjectResourceType);
        ASSERT_TRUE(res);

        res->SetResourceType(ClassId::TestResource);
        ASSERT_TRUE(resources->AddResourceType(interface_pointer_cast<CORE_NS::IResourceType>(res)));

        resources->SetFileManager(CORE_NS::IFileManager::Ptr(&GetTestEnv()->engine->GetFileManager()));
        context = GetObjectRegistry().Create(META_NS::ClassId::Object);
        context2 = GetObjectRegistry().Create(META_NS::ClassId::Object);
    }
    void TearDown() override {}

    CORE_NS::IResource::Ptr CreateTestResource(const CORE_NS::ResourceId& id)
    {
        auto res = GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::ObjectResource);
        if (auto i = interface_cast<IObjectResource>(res)) {
            i->SetResourceType(ClassId::TestResource);
        }
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(res)) {
            i->SetResourceId({ id, context });
        }
        return res;
    }

    CORE_NS::ResourceContextPtr context;
    CORE_NS::ResourceContextPtr context2;
    CORE_NS::IResourceManager::Ptr resources;
};

struct Listener : CORE_NS::IResourceManager::IResourceListener {
    void OnResourceEvent(CORE_NS::IResourceManager::IResourceListener::EventType type,
        const BASE_NS::array_view<const CORE_NS::ResourceIdContext>& ids) override
    {
        if (type == CORE_NS::IResourceManager::IResourceListener::EventType::ADDED) {
            for (auto&& v : ids) {
                added.push_back(v.id);
            }
        } else {
            for (auto&& v : ids) {
                removed.push_back(v.id);
            }
        }
    }
    BASE_NS::vector<CORE_NS::ResourceId> added;
    BASE_NS::vector<CORE_NS::ResourceId> removed;
};

/**
 * @tc.name: AddWithObject
 * @tc.desc: Tests for Add With Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, AddWithObject, testing::ext::TestSize.Level1)
{
    BASE_NS::shared_ptr<Listener> listener(new Listener);
    resources->AddListener(listener);
    resources->AddListener(listener); // again, should be a no-op
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    EXPECT_THAT(listener->added, testing::ElementsAre(CORE_NS::ResourceId("app://test.scene")));
    auto queried = resources->GetResource(CORE_NS::ResourceIdContext("app://test.scene", context));
    ASSERT_TRUE(queried);
    EXPECT_EQ(queried, res);
    resources->RemoveListener(listener);
}

/**
 * @tc.name: ResourceTypes
 * @tc.desc: Test resource type APIs
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ResourceTypes, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(resources->GetResourceTypes().empty());
    EXPECT_TRUE(resources->GetResourceGroups(context).empty());
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    EXPECT_FALSE(resources->GetResourceGroups(context).empty());
    EXPECT_TRUE(resources->GetResourceGroups(nullptr).empty());
    EXPECT_TRUE(resources->GetResourceGroups(context2).empty());
}

/**
 * @tc.name: ReloadResource
 * @tc.desc: Test ReloadResource API.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ReloadResource, testing::ext::TestSize.Level1)
{
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    auto groupsBefore = resources->GetResourceGroups(context);
    EXPECT_FALSE(resources->ReloadResource(res)); // ObjectResource fails to reload
    auto groupsAfter = resources->GetResourceGroups(context);
    EXPECT_THAT(groupsBefore, ::testing::ElementsAreArray(groupsAfter));
}

/**
 * @tc.name: PurgeGroup
 * @tc.desc: Test PurgeGroup API.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, PurgeGroup, testing::ext::TestSize.Level1)
{
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    auto groups = resources->GetResourceGroups(context);
    EXPECT_EQ(resources->PurgeGroup("invalid_name", context), 0);
    for (auto&& group : groups) {
        EXPECT_GT(resources->PurgeGroup(group, context), 0);
    }
}

/**
 * @tc.name: ResourceQuery
 * @tc.desc: Test IResourceQuery API.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ResourceQuery, testing::ext::TestSize.Level1)
{
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    auto query = interface_cast<IResourceQuery>(resources);
    ASSERT_TRUE(query);
    auto groups = resources->GetResourceGroups(context);
    for (auto&& group : groups) {
        const auto match = CORE_NS::MatchingResourceId(group);
        auto alive = query->FindAliveResources({ match }, context);
        EXPECT_FALSE(alive.empty());
        EXPECT_EQ(query->GetAliveCount({ match }, context), alive.size());
    }
}

/**
 * @tc.name: GetResourceHandle
 * @tc.desc: Test GetResourceHandle API.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, GetResourceHandle, testing::ext::TestSize.Level1)
{
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    auto owned = interface_cast<IOwnedResourceGroups>(resources);
    ASSERT_TRUE(owned);
    auto groups = resources->GetResourceGroups(context);
    for (auto&& group : groups) {
        EXPECT_TRUE(owned->GetGroupHandle(group, context));
    }
}

static void MakeEmptyJsonFile()
{
    // make sure we have json file to load
    TestSerialiser ser;
    auto type = GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::ObjectResource);
    if (auto i = interface_cast<IObjectResource>(type)) {
        i->SetResourceType(ClassId::TestResource);
    }
    ASSERT_TRUE(ser.Export(type));
    ser.Dump("app://test.json");
}

/**
 * @tc.name: AddWithoutObject
 * @tc.desc: Tests for Add Without Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, AddWithoutObject, testing::ext::TestSize.Level1)
{
    MakeEmptyJsonFile();
    BASE_NS::shared_ptr<Listener> listener(new Listener);
    resources->AddListener(listener);
    CORE_NS::ResourceIdContext ctx { "test", context };
    ASSERT_TRUE(resources->AddResource(ctx, ClassId::TestResource.Id().ToUid(), "app://test.json"));
    EXPECT_THAT(listener->added, testing::ElementsAre(CORE_NS::ResourceId("test")));
    auto queried = resources->GetResource(ctx);
    ASSERT_TRUE(queried);
}

/**
 * @tc.name: OptionsData
 * @tc.desc: Tests for Options Data. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, OptionsData, testing::ext::TestSize.Level1)
{
    MakeEmptyJsonFile();
    auto opts = GetObjectRegistry().Create<IObjectResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
    ASSERT_TRUE(opts);
    opts->SetProperty("test", uint32_t(2));
    ASSERT_TRUE(resources->AddResource({ "test", context }, ClassId::TestResource.Id().ToUid(), "app://test.json",
        interface_pointer_cast<CORE_NS::IResourceOptions>(opts)));
    auto queried = resources->GetResource(CORE_NS::ResourceIdContext("test", context));
    ASSERT_TRUE(queried);
    auto info = resources->GetResourceInfo({ "test", context });
    EXPECT_EQ(info.id, "test");
    EXPECT_EQ(info.type, ClassId::TestResource.Id().ToUid());
    EXPECT_EQ(info.path, "app://test.json");
    auto nopts = GetObjectRegistry().Create<IObjectResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
    ASSERT_TRUE(nopts);
    MemFile memopts(info.optionData);
    EXPECT_TRUE(nopts->Load(memopts, nullptr, nullptr));
    auto p = nopts->GetProperty<uint32_t>("test");
    ASSERT_TRUE(p);
    EXPECT_EQ(p->GetValue(), 2);
}

/**
 * @tc.name: ExportImportAll
 * @tc.desc: Tests for Export Import All. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ExportImportAll, testing::ext::TestSize.Level1)
{
    BASE_NS::vector<CORE_NS::ResourceId> exp;
    for (int i = 0; i != 10; ++i) {
        BASE_NS::string name("test_" + BASE_NS::to_string(i));
        exp.push_back(CORE_NS::ResourceId(name));
        auto res = CreateTestResource(name);
        ASSERT_TRUE(resources->AddResource(res, "app://" + name + ".json"));
    }
    ASSERT_EQ(resources->Export("app://test.resources", context), CORE_NS::IResourceManager::Result::OK);
    resources->RemoveAllResources();
    BASE_NS::shared_ptr<Listener> listener(new Listener);
    resources->AddListener(listener);
    ASSERT_EQ(resources->Import("app://test.resources", context), CORE_NS::IResourceManager::Result::OK);
    auto res = resources->GetResources(context);
    EXPECT_EQ(res.size(), 10);
    EXPECT_THAT(listener->added, testing::UnorderedElementsAreArray(exp));
}

/**
 * @tc.name: ExportImportGroups
 * @tc.desc: Tests for Export Import Groups. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ExportImportGroups, testing::ext::TestSize.Level1)
{
    BASE_NS::vector<BASE_NS::string> groups { "hips", "hops", "some", "" };
    for (auto&& g : groups) {
        for (int i = 0; i != 10; ++i) {
            BASE_NS::string name("test_" + BASE_NS::to_string(i));
            auto res = CreateTestResource(CORE_NS::ResourceId(name, g));
            ASSERT_TRUE(resources->AddResource(res, "app://" + name + ".json"));
        }
    }
    for (auto&& g : groups) {
        ASSERT_EQ(resources->Export("app://test_" + g + ".resources", { CORE_NS::MatchingResourceId(g) }, context),
            CORE_NS::IResourceManager::Result::OK);
    }
    resources->RemoveAllResources();
    int i = 0;
    for (auto&& g : groups) {
        ASSERT_EQ(resources->Import("app://test_" + g + ".resources", context), CORE_NS::IResourceManager::Result::OK);
        auto res = resources->GetResources(context);
        EXPECT_EQ(res.size(), ++i * 10);
        auto gres = resources->GetResources(g, context);
        EXPECT_EQ(gres.size(), 10);
    }

    EXPECT_TRUE(resources->GetResource(CORE_NS::ResourceIdContext("test_1", context)));
    EXPECT_TRUE(resources->GetResource(CORE_NS::ResourceIdContext({ "test_1", "hops" }, context)));
    EXPECT_TRUE(resources->GetResource(CORE_NS::ResourceIdContext({ "test_1", "some" }, context)));

    resources->RemoveResource(CORE_NS::ResourceIdContext({ "test_1", "hops" }, context));
    EXPECT_TRUE(resources->GetResource(CORE_NS::ResourceIdContext("test_1", context)));
    EXPECT_FALSE(resources->GetResource(CORE_NS::ResourceIdContext({ "test_1", "hops" }, context)));
    EXPECT_TRUE(resources->GetResource(CORE_NS::ResourceIdContext({ "test_1", "some" }, context)));

    resources->RemoveGroup("some", context);
    EXPECT_TRUE(resources->GetResource(CORE_NS::ResourceIdContext("test_1", context)));
    EXPECT_FALSE(resources->GetResource(CORE_NS::ResourceIdContext({ "test_1", "hops" }, context)));
    EXPECT_FALSE(resources->GetResource(CORE_NS::ResourceIdContext({ "test_1", "some" }, context)));
    EXPECT_EQ(resources->GetResources("some", context).size(), 0);

    EXPECT_EQ(resources->GetResourceInfos("hips", context).size(), 10);
}

/**
 * @tc.name: Rename
 * @tc.desc: Tests for Rename. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, Rename, testing::ext::TestSize.Level1)
{
    BASE_NS::shared_ptr<Listener> listener(new Listener);
    resources->AddListener(listener);
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    EXPECT_THAT(listener->added, testing::ElementsAre(CORE_NS::ResourceId("app://test.scene")));
    {
        auto queried = resources->GetResource(CORE_NS::ResourceIdContext("app://test.scene", context));
        ASSERT_TRUE(queried);
        EXPECT_EQ(queried, res);
    }
    listener->added.clear();
    resources->RenameResource({ "app://test.scene", context }, { "ggg", context });
    EXPECT_THAT(listener->removed, testing::ElementsAre(CORE_NS::ResourceId("app://test.scene")));
    EXPECT_THAT(listener->added, testing::ElementsAre(CORE_NS::ResourceId("ggg")));
    {
        EXPECT_FALSE(resources->GetResource({ CORE_NS::ResourceId("app://test.scene"), context }));
        auto queried = resources->GetResource(CORE_NS::ResourceIdContext("ggg", context));
        ASSERT_TRUE(queried);
        EXPECT_EQ(queried, res);
    }
}

/**
 * @tc.name: Purge
 * @tc.desc: Tests for Purge. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, Purge, testing::ext::TestSize.Level1)
{
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    {
        auto queried = resources->GetResource(CORE_NS::ResourceIdContext("app://test.scene", context));
        ASSERT_TRUE(queried);
        EXPECT_EQ(queried, res);
    }
    CORE_NS::IResource::WeakPtr w = res;
    res.reset();
    resources->PurgeResource({ "app://test.scene", context });
    EXPECT_TRUE(w.expired());
    {
        auto queried = resources->GetResource(CORE_NS::ResourceIdContext("app://test.scene", context));
        ASSERT_TRUE(queried);
    }
}

/**
 * @tc.name: ObjectResourceTypeName
 * @tc.desc: Tests that ObjectResourceType returns the correct resource name.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ObjectResourceTypeName, testing::ext::TestSize.Level1)
{
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::ObjectResourceType);
    ASSERT_TRUE(resType);
    EXPECT_EQ(resType->GetResourceName(), "ObjectResource");
}

/**
 * @tc.name: SaveResourceNullPtr
 * @tc.desc: Tests that SaveResource returns false when resource pointer is null.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, SaveResourceNullPtr, testing::ext::TestSize.Level1)
{
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::ObjectResourceType);
    ASSERT_TRUE(resType);
    if (auto objRes = interface_cast<IObjectResource>(resType)) {
        objRes->SetResourceType(ClassId::TestResource);
    }

    MemFile file;
    CORE_NS::ResourceId rid("test_save_null");
    CORE_NS::IResourceType::StorageInfo info { nullptr, &file, rid };
    EXPECT_FALSE(resType->SaveResource(nullptr, info));
}

/**
 * @tc.name: LoadResourceTypeMismatch
 * @tc.desc: Tests that LoadResource returns null when loaded resource type does not match.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, LoadResourceTypeMismatch, testing::ext::TestSize.Level1)
{
    // Serialize an ObjectResource with TestResource type
    TestSerialiser ser;
    auto obj = GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::ObjectResource);
    ASSERT_TRUE(obj);
    if (auto i = interface_cast<IObjectResource>(obj)) {
        i->SetResourceType(ClassId::TestResource);
    }
    ASSERT_TRUE(ser.Export(obj));
    auto data = ser.GetData();

    // Create an ObjectResourceType configured for a different type
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::ObjectResourceType);
    ASSERT_TRUE(resType);
    if (auto i = interface_cast<IObjectResource>(resType)) {
        i->SetResourceType(::META_NS::ClassId::Object);
    }

    MemFile payload(data);
    CORE_NS::ResourceId rid("test_mismatch");
    CORE_NS::IResourceType::StorageInfo info { nullptr, &payload, rid, "", resources, context };
    auto result = resType->LoadResource(info);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SetPropertyOverridesExisting
 * @tc.desc: Tests that SetProperty replaces an existing property with the same name.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, SetPropertyOverridesExisting, testing::ext::TestSize.Level1)
{
    auto opts = GetObjectRegistry().Create<IObjectResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
    ASSERT_TRUE(opts);

    opts->SetProperty("test", uint32_t(1));
    auto p1 = opts->GetProperty<uint32_t>("test");
    ASSERT_TRUE(p1);
    EXPECT_EQ(p1->GetValue(), 1);

    // Override with same name - should remove old and add new
    opts->SetProperty("test", uint32_t(2));
    auto p2 = opts->GetProperty<uint32_t>("test");
    ASSERT_TRUE(p2);
    EXPECT_EQ(p2->GetValue(), 2);
}

/**
 * @tc.name: ObjectResourceReloadResource
 * @tc.desc: Tests that ObjectResourceType ReloadResource returns false.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ObjectResourceReloadResource, testing::ext::TestSize.Level1)
{
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::ObjectResourceType);
    ASSERT_TRUE(resType);

    CORE_NS::ResourceId rid;
    CORE_NS::IResourceType::StorageInfo info { nullptr, nullptr, rid };
    CORE_NS::IResource::Ptr res;
    EXPECT_FALSE(resType->ReloadResource(info, res));
}

} // namespace UTest

META_END_NAMESPACE()
