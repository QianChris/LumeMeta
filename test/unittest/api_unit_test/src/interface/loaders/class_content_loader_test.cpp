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
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/loaders/intf_class_content_loader.h>
#include <meta/interface/property/property_events.h>

#include "helpers/testing_objects.h"

META_BEGIN_NAMESPACE()
namespace UTest {

/**
 * @tc.name: CreateInstance
 * @tc.desc: Tests that ClassContentLoader can be created from the registry.
 * @tc.type: FUNC
 */
UNIT_TEST(API_ClassContentLoaderTest, CreateInstance, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IClassContentLoader>(ClassId::ClassContentLoader);
    ASSERT_TRUE(loader);
}

/**
 * @tc.name: CreateWithEmptyClassId
 * @tc.desc: Tests that Create returns null when ClassId is empty.
 * @tc.type: FUNC
 */
UNIT_TEST(API_ClassContentLoaderTest, CreateWithEmptyClassId, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IClassContentLoader>(ClassId::ClassContentLoader);
    ASSERT_TRUE(loader);

    // Default ClassId should be empty, so Create should return null
    auto result = loader->Create(nullptr);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: CreateWithValidClassId
 * @tc.desc: Tests that Create returns a valid object when ClassId is set to a known class.
 * @tc.type: FUNC
 */
UNIT_TEST(API_ClassContentLoaderTest, CreateWithValidClassId, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IClassContentLoader>(ClassId::ClassContentLoader);
    ASSERT_TRUE(loader);

    // Set ClassId to Object class
    loader->ClassId()->SetValue(ClassId::Object);

    auto result = loader->Create(nullptr);
    ASSERT_TRUE(result);
}

/**
 * @tc.name: CreateWithInvalidClassId
 * @tc.desc: Tests that Create returns null when ClassId is set to an unknown UID.
 * @tc.type: FUNC
 */
UNIT_TEST(API_ClassContentLoaderTest, CreateWithInvalidClassId, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IClassContentLoader>(ClassId::ClassContentLoader);
    ASSERT_TRUE(loader);

    // Set ClassId to an unknown UID
    loader->ClassId()->SetValue(BASE_NS::Uid { "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee" });

    auto result = loader->Create(nullptr);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: ReloadFiresContentChanged
 * @tc.desc: Tests that Reload fires the ContentChanged event.
 * @tc.type: FUNC
 */
UNIT_TEST(API_ClassContentLoaderTest, ReloadFiresContentChanged, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IClassContentLoader>(ClassId::ClassContentLoader);
    ASSERT_TRUE(loader);

    int changeCount = 0;
    loader->ContentChanged()->AddHandler(MakeCallback<IOnChanged>([&changeCount] { changeCount++; }));

    loader->Reload();
    EXPECT_EQ(changeCount, 1);

    loader->Reload();
    EXPECT_EQ(changeCount, 2);
}

/**
 * @tc.name: ClassIdChangeTriggersContentChanged
 * @tc.desc: Tests that changing ClassId property triggers ContentChanged event.
 * @tc.type: FUNC
 */
UNIT_TEST(API_ClassContentLoaderTest, ClassIdChangeTriggersContentChanged, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IClassContentLoader>(ClassId::ClassContentLoader);
    ASSERT_TRUE(loader);

    int changeCount = 0;
    loader->ContentChanged()->AddHandler(MakeCallback<IOnChanged>([&changeCount] { changeCount++; }));

    // Changing the ClassId property should fire ContentChanged
    loader->ClassId()->SetValue(ClassId::Object);
    EXPECT_GE(changeCount, 1);
}

/**
 * @tc.name: CreateMultipleInstances
 * @tc.desc: Tests that multiple Create calls produce distinct instances.
 * @tc.type: FUNC
 */
UNIT_TEST(API_ClassContentLoaderTest, CreateMultipleInstances, testing::ext::TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IClassContentLoader>(ClassId::ClassContentLoader);
    ASSERT_TRUE(loader);

    loader->ClassId()->SetValue(ClassId::Object);

    auto obj1 = loader->Create(nullptr);
    auto obj2 = loader->Create(nullptr);
    ASSERT_TRUE(obj1);
    ASSERT_TRUE(obj2);
    EXPECT_NE(obj1.get(), obj2.get());
}

} // namespace UTest
META_END_NAMESPACE()
