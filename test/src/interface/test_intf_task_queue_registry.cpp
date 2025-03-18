/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#include <tester.h>
#include <type_traits>

#include <gtest/gtest.h>

#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_task_queue_registry.h>

META_BEGIN_NAMESPACE()

namespace Test {

constexpr BASE_NS::Uid CustomTaskQueueId { "7a55939d-3b89-479f-9e6a-40a13b80434d" };

TEST(UnitTest_TaskQueueRegistryTest, Register)
{
    auto& taskqRegistry = GetTaskQueueRegistry();
    auto& objRegistry = GetObjectRegistry();

    auto queue = objRegistry.Create<ITaskQueue>(ClassId::PollingTaskQueue);
    ASSERT_NE(queue, nullptr);

    // Register unexisting null
    EXPECT_FALSE(taskqRegistry.RegisterTaskQueue({}, CustomTaskQueueId));
    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));

    // Register valid
    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.RegisterTaskQueue(queue, CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));

    // Register again
    EXPECT_TRUE(taskqRegistry.RegisterTaskQueue(queue, CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));

    // Register null, this should remove
    EXPECT_TRUE(taskqRegistry.RegisterTaskQueue({}, CustomTaskQueueId));
    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));

    EXPECT_TRUE(taskqRegistry.UnregisterAllTaskQueues());
}

TEST(UnitTest_TaskQueueRegistryTest, Replace)
{
    auto& taskqRegistry = GetTaskQueueRegistry();
    auto& objRegistry = GetObjectRegistry();

    auto queue1 = objRegistry.Create<ITaskQueue>(ClassId::PollingTaskQueue);
    auto queue2 = objRegistry.Create<ITaskQueue>(ClassId::PollingTaskQueue);
    ASSERT_NE(queue1, nullptr);
    ASSERT_NE(queue2, nullptr);

    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.RegisterTaskQueue(queue1, CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));
    EXPECT_EQ(taskqRegistry.GetTaskQueue(CustomTaskQueueId), queue1);

    // Replace
    EXPECT_TRUE(taskqRegistry.RegisterTaskQueue(queue2, CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));
    EXPECT_EQ(taskqRegistry.GetTaskQueue(CustomTaskQueueId), queue2);

    EXPECT_TRUE(taskqRegistry.UnregisterAllTaskQueues());
}

TEST(UnitTest_TaskQueueRegistryTest, UnRegister)
{
    auto& taskqRegistry = GetTaskQueueRegistry();
    auto& objRegistry = GetObjectRegistry();

    auto queue = objRegistry.Create<ITaskQueue>(ClassId::PollingTaskQueue);
    ASSERT_NE(queue, nullptr);

    // Register valid
    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.RegisterTaskQueue(queue, CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.UnregisterTaskQueue(CustomTaskQueueId));
    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));

    EXPECT_FALSE(taskqRegistry.UnregisterTaskQueue(CustomTaskQueueId));
    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));

    EXPECT_FALSE(taskqRegistry.UnregisterTaskQueue({}));
    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));

    EXPECT_TRUE(taskqRegistry.UnregisterAllTaskQueues());
}

} // namespace Test

META_END_NAMESPACE()
