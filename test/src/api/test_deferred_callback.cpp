/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#include <tester.h>

#include <gtest/gtest.h>

#include <meta/api/deferred_callback.h>
#include <meta/api/property/property_event_handler.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/object_macros.h>
#include <meta/interface/property/intf_property.h>

#include "util.h"

META_BEGIN_NAMESPACE()

namespace Test {
using namespace testing;

namespace {

struct IOnTestInfo {
    constexpr static BASE_NS::Uid UID { "3e55eee7-f0e4-4363-b4fc-65d8b3574a4e" };
    constexpr static char const* NAME { "OnTest" };
};
using IOnTest = SimpleEvent<IOnTestInfo, void(int)>;

using Callable = SimpleEvent<IOnTestInfo, void()>::InterfaceType;

} // namespace

TEST(DeferredCallbackTest, CallWithCallable)
{
    auto& registry = META_NS::GetObjectRegistry();

    int count = 0;
    auto q = registry.Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto callback = MakeDeferred<Callable>([&] { ++count; }, q);

    q->ProcessTasks();
    EXPECT_EQ(count, 0);

    callback->Invoke();
    callback->Invoke();

    EXPECT_EQ(count, 0);

    q->ProcessTasks();
    EXPECT_EQ(count, 2);
}

TEST(DeferredCallbackTest, CallWithEvent)
{
    auto& registry = META_NS::GetObjectRegistry();

    int count = 0;
    auto q = registry.Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto callback = MakeDeferred<IOnTest>([&](int i) { count += i; }, q);

    q->ProcessTasks();
    EXPECT_EQ(count, 0);

    callback->Invoke(1);
    callback->Invoke(2);

    EXPECT_EQ(count, 0);

    q->ProcessTasks();
    EXPECT_EQ(count, 3);
}

TEST(DeferredCallbackTest, PropertyOnChange)
{
    auto& registry = META_NS::GetObjectRegistry();

    auto p = META_NS::ConstructProperty<int>(registry, "P", {});
    ASSERT_TRUE(p);

    int count = 0;
    auto q = registry.Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);

    auto t = p->OnChanged()->AddHandler(MakeDeferred<IOnChanged>([&] { ++count; }, q));

    p->SetValue(1);

    EXPECT_EQ(count, 0);
    q->ProcessTasks();
    EXPECT_EQ(count, 1);

    p->OnChanged()->RemoveHandler(t);

    p->SetValue(2);
    q->ProcessTasks();
    EXPECT_EQ(count, 1);
}

TEST(DeferredCallbackTest, PropertyChangedHandler)
{
    auto& registry = META_NS::GetObjectRegistry();

    auto p = META_NS::ConstructProperty<int>(registry, "P", {});
    ASSERT_TRUE(p);

    int count = 0;
    auto q = registry.Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);

    PropertyChangedEventHandler handler_;
    handler_.Subscribe(
        p, [&] { ++count; }, q);

    p->SetValue(1);

    EXPECT_EQ(count, 0);
    q->ProcessTasks();
    EXPECT_EQ(count, 1);
}

TEST(DeferredCallbackTest, PropertyChangedHandlerId)
{
    auto& registry = META_NS::GetObjectRegistry();

    auto p = META_NS::ConstructProperty<int>(registry, "P", {});
    ASSERT_TRUE(p);

    int count = 0;
    auto q = registry.Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto queueId = interface_cast<IObjectInstance>(q)->GetInstanceId();
    META_NS::GetTaskQueueRegistry().RegisterTaskQueue(q, queueId.ToUid());

    PropertyChangedEventHandler handler_;
    handler_.Subscribe(
        p, [&] { ++count; }, queueId.ToUid());

    p->SetValue(1);

    EXPECT_EQ(count, 0);
    q->ProcessTasks();
    EXPECT_EQ(count, 1);
    META_NS::GetTaskQueueRegistry().UnregisterTaskQueue(queueId.ToUid());
}

} // namespace Test

META_END_NAMESPACE()
