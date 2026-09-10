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

#ifndef META_SRC_RUN_ON_QUEUE_H
#define META_SRC_RUN_ON_QUEUE_H

#include <meta/api/make_callback.h>
#include <meta/base/namespace.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Whether the calling thread is the one `queue` executes its tasks on.
 *
 * Asked of the queue rather than tracked by the caller: a queue knows, and a thread id remembered
 * elsewhere would only be a guess about who runs it. The answer is what keeps @ref RunOnQueueAndWait
 * from waiting on a queue from its own thread, which deadlocks.
 */
inline bool IsQueueExecutionThread(const ITaskQueue& queue)
{
    if (auto info = interface_cast<ITaskQueueThreadInfo>(&queue)) {
        return info->CurrentThreadIsExecutionThread();
    }
    // A queue that does not say gets the next best answer: whether this thread is running a task of that
    // queue right now.
    return GetTaskQueueRegistry().GetCurrentTaskQueue().get() == &queue;
}

/**
 * @brief Run `fn` where the things it touches belong: here, when there is no queue or this already is
 *        that thread, and otherwise on the queue, waited for so the caller's result still means
 *        something by the time this returns.
 *
 * For work that must land on one particular thread — writing properties, which belong to one thread at a
 * time — while being reachable from any of them.
 */
template<typename Fn>
void RunOnQueueAndWait(const ITaskQueue::Ptr& queue, Fn&& fn)
{
    if (!queue || IsQueueExecutionThread(*queue)) {
        fn();
        return;
    }
    // By reference: the wait below outlives the task, so nothing has to be copied into it.
    if (auto future = queue->AddWaitableTask(MakeCallback<ITaskQueueWaitableTask>([&fn]() -> IAny::Ptr {
            fn();
            return nullptr;
        }))) {
        future->Wait();
    }
}

META_END_NAMESPACE()

#endif
