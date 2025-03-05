/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#include <tester.h>

#include <meta/api/timer.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>

#include "test_macros.h"

META_BEGIN_NAMESPACE()

namespace Test {
using namespace testing;

class TimerTests : public ::testing::Test {
    void SetUp() override
    {
        queue_ = GetObjectRegistry().Create<IThreadedTaskQueue>(ClassId::ThreadedTaskQueue);
        queueId_ = interface_cast<IObjectInstance>(queue_)->GetInstanceId();
        GetTaskQueueRegistry().RegisterTaskQueue(queue_, queueId_.ToUid());
    }

    void TearDown() override
    {
        GetTaskQueueRegistry().UnregisterTaskQueue(queueId_.ToUid());
    }

protected:
    ITaskQueue::Ptr queue_;
    InstanceId queueId_;
};

TEST_F(TimerTests, Ctor)
{
    {
        std::atomic<int> count = 0;
        Timer t(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::RECURRING, queueId_.ToUid());
        EXPECT_TRUE(t.IsRunning());
        EXPECT_EQ_TIMED(350, count, 3);
    }
    {
        std::atomic<int> count = 0;
        Timer t(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::RECURRING, queue_);
        EXPECT_TRUE(t.IsRunning());
        EXPECT_EQ_TIMED(350, count, 3);
    }
}

TEST_F(TimerTests, Moving)
{
    Timer t(
        TimeSpan::Milliseconds(20), [] {}, Timer::RECURRING, queueId_.ToUid());
    EXPECT_TRUE(t.IsRunning());
    Timer t2 = BASE_NS::move(t);
    EXPECT_FALSE(t.IsRunning());
    EXPECT_TRUE(t2.IsRunning());
}

TEST_F(TimerTests, Start)
{
    {
        std::atomic<int> count = 0;
        Timer t;
        EXPECT_TRUE(t.Start(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::RECURRING, queueId_.ToUid()));
        EXPECT_TRUE(t.IsRunning());
        EXPECT_EQ_TIMED(350, count, 3);
    }
    {
        std::atomic<int> count = 0;
        Timer t;
        EXPECT_TRUE(t.Start(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::RECURRING, queue_));
        EXPECT_TRUE(t.IsRunning());
        EXPECT_EQ_TIMED(350, count, 3);
    }
}

TEST_F(TimerTests, Stop)
{
    {
        std::atomic<int> count = 0;
        Timer t;
        EXPECT_TRUE(t.Start(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::RECURRING, queueId_.ToUid()));
        EXPECT_TRUE(t.IsRunning());
        EXPECT_EQ_TIMED(150, count, 1);
        t.Stop();
        META_WAIT_TIMED(150);
        EXPECT_EQ(count, 1);
        EXPECT_FALSE(t.IsRunning());
    }
    {
        std::atomic<int> count = 0;
        {
            Timer t;
            EXPECT_TRUE(t.Start(
                TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::RECURRING, queueId_.ToUid()));
            EXPECT_TRUE(t.IsRunning());
            EXPECT_EQ_TIMED(150, count, 1);
        }
        META_WAIT_TIMED(150);
        EXPECT_EQ(count, 1);
    }
}

TEST_F(TimerTests, SingleShot)
{
    {
        std::atomic<int> count = 0;
        Timer t;
        EXPECT_TRUE(t.Start(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::SINGLE_SHOT, queueId_.ToUid()));
        EXPECT_TRUE(t.IsRunning());
        EXPECT_EQ_TIMED(150, count, 1);
        META_WAIT_TIMED(150);
        EXPECT_EQ(count, 1);
        EXPECT_FALSE(t.IsRunning());
    }
    {
        std::atomic<int> count = 0;
        SingleShotTimer(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, queueId_.ToUid());
        EXPECT_EQ_TIMED(150, count, 1);
        META_WAIT_TIMED(150);
        EXPECT_EQ(count, 1);
    }
}

TEST_F(TimerTests, Detach)
{
    std::atomic<int> count = 0;
    {
        Timer t;
        EXPECT_TRUE(t.Start(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::SINGLE_SHOT, queueId_.ToUid()));
        t.Detach();
        EXPECT_FALSE(t.IsRunning());
    }
    EXPECT_EQ_TIMED(150, count, 1);
    META_WAIT_TIMED(150);
    EXPECT_EQ(count, 1);
}

TEST_F(TimerTests, BadQueue)
{
    {
        Timer t(
            TimeSpan::Milliseconds(20), [] {}, Timer::RECURRING,
            BASE_NS::Uid { "b63ceb45-bdb3-4f23-bd95-bb640eba9144" });
        EXPECT_TRUE(!t.IsRunning());
    }
    {
        Timer t;
        EXPECT_FALSE(t.Start(
            TimeSpan::Milliseconds(20), [] {}, Timer::RECURRING,
            BASE_NS::Uid { "b63ceb45-bdb3-4f23-bd95-bb640eba9144" }));
        EXPECT_TRUE(!t.IsRunning());
    }
    {
        Timer t(
            TimeSpan::Milliseconds(20), [] {}, Timer::RECURRING, nullptr);
        EXPECT_TRUE(!t.IsRunning());
    }
    {
        Timer t;
        EXPECT_FALSE(t.Start(
            TimeSpan::Milliseconds(20), [] {}, Timer::RECURRING, nullptr));
        EXPECT_TRUE(!t.IsRunning());
    }
}
/*
TEST_F(TimerTests, Debug)
{
    Timer t;
    auto start = std::chrono::steady_clock::now();
    EXPECT_TRUE(t.Start(
        TimeSpan::Milliseconds(20),
        [start] {
            auto diff = std::chrono::steady_clock::now() - start;
            CORE_LOG_W("hit %lld", std::chrono::duration_cast<std::chrono::milliseconds>(diff).count());
        },
        Timer::RECURRING, queueId_));

    META_WAIT_TIMED(200);
}
*/
} // namespace Test

META_END_NAMESPACE()
