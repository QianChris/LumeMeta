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

#include <test_framework.h>

#include <meta/api/make_callback.h>
#include <meta/api/object.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/loaders/intf_file_content_loader.h>
#include <meta/interface/property/property_events.h>

#include "helpers/serialisation_utils.h"
#include "helpers/testing_objects.h"

META_BEGIN_NAMESPACE()
namespace UTest {

namespace {

CORE_NS::IFile::Ptr CreateValidJsonFile()
{
    TestSerialiser ser;
    ser.Export(CreateInstance(ClassId::Object));
    return CORE_NS::IFile::Ptr(new MemFile(ser.GetData()));
}

CORE_NS::IFile::Ptr CreateInvalidJsonFile()
{
    BASE_NS::vector<uint8_t> data = { '{', 'n', 'o', 't', ' ', 'j', 's', 'o', 'n' };
    return CORE_NS::IFile::Ptr(new MemFile(data));
}

} // namespace

/**
 * @tc.name: CreateInstance
 * @tc.desc: Tests that JsonContentLoader can be created from the registry.
 * @tc.type: FUNC
 */
UNIT_TEST(API_JsonContentLoaderTest, CreateInstance, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IFileContentLoader>(ClassId::JsonContentLoader);
    ASSERT_TRUE(loader);
}

/**
 * @tc.name: CreateWithoutFile
 * @tc.desc: Tests that Create returns null when no file is set.
 * @tc.type: FUNC
 */
UNIT_TEST(API_JsonContentLoaderTest, CreateWithoutFile, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IFileContentLoader>(ClassId::JsonContentLoader);
    ASSERT_TRUE(loader);

    auto result = loader->Create(nullptr);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: CreateWithValidFile
 * @tc.desc: Tests that Create returns a valid object when a valid JSON file is set.
 * @tc.type: FUNC
 */
UNIT_TEST(API_JsonContentLoaderTest, CreateWithValidFile, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IFileContentLoader>(ClassId::JsonContentLoader);
    ASSERT_TRUE(loader);

    EXPECT_TRUE(loader->SetFile(CreateValidJsonFile()));

    auto result = loader->Create(nullptr);
    ASSERT_TRUE(result);
}

/**
 * @tc.name: CreateWithInvalidJson
 * @tc.desc: Tests that Create returns null when file contains invalid JSON.
 * @tc.type: FUNC
 */
UNIT_TEST(API_JsonContentLoaderTest, CreateWithInvalidJson, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IFileContentLoader>(ClassId::JsonContentLoader);
    ASSERT_TRUE(loader);

    loader->SetFile(CreateInvalidJsonFile());

    auto result = loader->Create(nullptr);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SetFileTriggersContentChanged
 * @tc.desc: Tests that SetFile fires the ContentChanged event.
 * @tc.type: FUNC
 */
UNIT_TEST(API_JsonContentLoaderTest, SetFileTriggersContentChanged, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IFileContentLoader>(ClassId::JsonContentLoader);
    ASSERT_TRUE(loader);

    int changeCount = 0;
    loader->ContentChanged()->AddHandler(MakeCallback<IOnChanged>([&changeCount] { changeCount++; }));

    loader->SetFile(CreateValidJsonFile());
    EXPECT_GE(changeCount, 1);
}

/**
 * @tc.name: ReloadFiresContentChanged
 * @tc.desc: Tests that Reload fires the ContentChanged event.
 * @tc.type: FUNC
 */
UNIT_TEST(API_JsonContentLoaderTest, ReloadFiresContentChanged, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IFileContentLoader>(ClassId::JsonContentLoader);
    ASSERT_TRUE(loader);

    int changeCount = 0;
    loader->ContentChanged()->AddHandler(MakeCallback<IOnChanged>([&changeCount] { changeCount++; }));

    loader->Reload();
    EXPECT_EQ(changeCount, 1);

    loader->Reload();
    EXPECT_EQ(changeCount, 2);
}

/**
 * @tc.name: CachedModeCreateMultiple
 * @tc.desc: Tests that cached mode allows multiple Create calls from the same file.
 * @tc.type: FUNC
 */
UNIT_TEST(API_JsonContentLoaderTest, CachedModeCreateMultiple, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IFileContentLoader>(ClassId::JsonContentLoader);
    ASSERT_TRUE(loader);

    loader->SetFile(CreateValidJsonFile());
    loader->Cached()->SetValue(true);

    auto obj1 = loader->Create(nullptr);
    auto obj2 = loader->Create(nullptr);
    ASSERT_TRUE(obj1);
    ASSERT_TRUE(obj2);
}

/**
 * @tc.name: CachedModeWithoutFile
 * @tc.desc: Tests that cached mode Create returns null when no file is set.
 * @tc.type: FUNC
 */
UNIT_TEST(API_JsonContentLoaderTest, CachedModeWithoutFile, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IFileContentLoader>(ClassId::JsonContentLoader);
    ASSERT_TRUE(loader);

    loader->Cached()->SetValue(true);

    auto result = loader->Create(nullptr);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: ReloadClearsCachedTree
 * @tc.desc: Tests that Reload clears the cached tree so next Create re-parses.
 * @tc.type: FUNC
 */
UNIT_TEST(API_JsonContentLoaderTest, ReloadClearsCachedTree, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IFileContentLoader>(ClassId::JsonContentLoader);
    ASSERT_TRUE(loader);

    loader->SetFile(CreateValidJsonFile());
    loader->Cached()->SetValue(true);

    // First create should parse and cache
    auto obj1 = loader->Create(nullptr);
    ASSERT_TRUE(obj1);

    // Reload clears cache
    loader->Reload();

    // Next create should re-parse (set a new valid file after reload)
    loader->SetFile(CreateValidJsonFile());
    auto obj2 = loader->Create(nullptr);
    ASSERT_TRUE(obj2);
}

/**
 * @tc.name: NonCachedModeMultipleCreates
 * @tc.desc: Tests that non-cached mode can create objects multiple times from the same file.
 * @tc.type: FUNC
 */
UNIT_TEST(API_JsonContentLoaderTest, NonCachedModeMultipleCreates, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IFileContentLoader>(ClassId::JsonContentLoader);
    ASSERT_TRUE(loader);

    loader->SetFile(CreateValidJsonFile());
    // Cached is false by default

    auto obj1 = loader->Create(nullptr);
    auto obj2 = loader->Create(nullptr);
    ASSERT_TRUE(obj1);
    ASSERT_TRUE(obj2);
}

/**
 * @tc.name: CachedPropertyChangeTriggersReload
 * @tc.desc: Tests that changing the Cached property triggers a Reload (ContentChanged).
 * @tc.type: FUNC
 */
UNIT_TEST(API_JsonContentLoaderTest, CachedPropertyChangeTriggersReload, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IFileContentLoader>(ClassId::JsonContentLoader);
    ASSERT_TRUE(loader);

    int changeCount = 0;
    loader->ContentChanged()->AddHandler(MakeCallback<IOnChanged>([&changeCount] { changeCount++; }));

    loader->Cached()->SetValue(true);
    EXPECT_GE(changeCount, 1);
}

} // namespace UTest
META_END_NAMESPACE()
