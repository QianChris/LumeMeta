#include <benchmark/benchmark.h>
#include <deque>

#include <meta/api/make_callback.h>
#include <meta/api/object.h>
#include <meta/base/namespace.h>
#include <meta/interface/intf_task_queue.h>

#include "property_utils.h"

META_BEGIN_NAMESPACE()
namespace benchmarks {

void PushToTaskQueue(benchmark::State& state)
{
    auto q = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto task = MakeCallback<ITaskQueueTask>([] { return false; });
    for (auto _ : state) {
        for (int i = 0; i != 200; ++i) {
            q->AddTask(task);
        }
        q->ProcessTasks();
    }
}

void PushToTaskQueueWithLongLastingTasks(benchmark::State& state)
{
    auto q = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto task = MakeCallback<ITaskQueueTask>([] { return false; });
    for (int i = 0; i != 20; ++i) {
        q->AddTask(task, TimeSpan::Seconds(10));
    }
    for (auto _ : state) {
        for (int i = 0; i != 200; ++i) {
            q->AddTask(task);
        }
        q->ProcessTasks();
    }
}

void PushToThreadedTaskQueue(benchmark::State& state)
{
    auto q = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    std::atomic<int> value {};
    auto task = MakeCallback<ITaskQueueTask>([&] {
        ++value;
        return false;
    });
    for (auto _ : state) {
        for (int i = 0; i != 200; ++i) {
            q->AddTask(task);
        }
        state.PauseTiming();
        while (value < 200) {
        }
        value = 0;
        state.ResumeTiming();
    }
}

void PushToThreadedTaskQueueLongLastingTasks(benchmark::State& state)
{
    auto q = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    auto dummytask = MakeCallback<ITaskQueueTask>([] { return false; });
    for (int i = 0; i != 20; ++i) {
        q->AddTask(dummytask, TimeSpan::Seconds(10));
    }
    std::atomic<int> value {};
    auto task = MakeCallback<ITaskQueueTask>([&] {
        ++value;
        return false;
    });
    for (auto _ : state) {
        for (int i = 0; i != 200; ++i) {
            q->AddTask(task);
        }
        state.PauseTiming();
        while (value < 200) {
        }
        value = 0;
        state.ResumeTiming();
    }
}

BENCHMARK(PushToTaskQueue);
BENCHMARK(PushToTaskQueueWithLongLastingTasks);
BENCHMARK(PushToThreadedTaskQueue);
BENCHMARK(PushToThreadedTaskQueueLongLastingTasks);

} // namespace benchmarks
META_END_NAMESPACE()
