/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <mutex>
#include <thread>

#include <base/containers/vector.h>

#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>

#include "future.h"
#include "object.h"
#include "task_queue.h"

META_BEGIN_NAMESPACE()

// notice, this is object only so we can construct it via object registry
class PollingTaskQueue
    : public IntroduceInterfaces<MetaObject, IPollingTaskQueue, ITaskQueueThreadInfo, TaskQueueImpl> {
    META_OBJECT(PollingTaskQueue, ClassId::PollingTaskQueue, IntroduceInterfaces)
public:
    using Token = ITaskQueue::Token;

    bool Build(const IMetadata::Ptr& data) override
    {
        bool ret = Super::Build(data);
        if (ret) {
            self_ = GetSelf<ITaskQueue>();
        }
        return ret;
    }
    uint64_t CurrentThread() const
    {
        return std::hash<std::thread::id>{}(std::this_thread::get_id());
    }
    bool SetExecutionThread()
    {
        auto id = CurrentThread();
        if (threadId_ != 0) {
            return (threadId_ == id);
        }
        threadId_ = id;
        return true;
    }
    bool CurrentThreadIsExecutionThread() const override
    {
        return CurrentThread() == threadId_;
    }

    bool InvokeTask(const ITaskQueueTask::Ptr& task) override
    {
        auto q = GetTaskQueueRegistry().SetCurrentTaskQueue(self_);
        auto ret = task->Invoke();
        GetTaskQueueRegistry().SetCurrentTaskQueue(q);
        return ret;
    }
    void CancelTask(Token token) override
    {
        TaskQueueImpl::CancelTask(token);
    }

    Token AddTask(ITaskQueueTask::Ptr p) override
    {
        return AddTask(BASE_NS::move(p), TimeSpan::Milliseconds(0));
    }

    Token AddTask(ITaskQueueTask::Ptr p, const TimeSpan& delay) override
    {
        return TaskQueueImpl::AddTask(BASE_NS::move(p), delay, Time() + delay);
    }

    IFuture::Ptr AddWaitableTask(ITaskQueueWaitableTask::Ptr p) override
    {
        IPromise::Ptr promise(new Promise);
        BASE_NS::shared_ptr<PromisedQueueTask> task(new PromisedQueueTask(BASE_NS::move(p), promise));
        auto f = task->GetFuture();
        if (!AddTask(BASE_NS::move(task))) {
            // The queue refused the task (it is shut down), so nothing will ever complete the promise.
            // Abandon it here, otherwise a caller waiting on the future would wait forever.
            promise->SetAbandoned();
        }
        return f;
    }

    void ProcessTasks() override
    {
        if (!SetExecutionThread()) {
            CORE_LOG_F("ProcessTasks called from different thread!");
            return;
        }
        TimeSpan ctime = Time();
        std::unique_lock lock{mutex_};
        if (processing_) {
            // A task is pumping the queue it runs on, which a nested event loop makes possible.
            // TaskQueueImpl drops the lock around a task, so this recurses rather than deadlocks,
            // and execThread_, execToken_ and currentlyExecutingRemoved are one set of members
            // shared by every pass. A nested pass clears them under the outer one, and the outer
            // pass then runs on believing no task is executing, which breaks CancelTask both ways:
            // a cancel aimed at the outer task matches nothing and returns without cancelling or
            // waiting, and a task that cancels itself waits for itself forever. Running nothing is
            // the deliberate cost, see IPollingTaskQueue::ProcessTasks for the way out.
            CORE_LOG_V("Reentrant call to ProcessTasks, ignored.");
            return;
        }
        // Keep this straight-line. Nothing between the two assignments returns, so the reset is
        // always reached. An early exit added here later would leave processing_ set, and that
        // turns every later pump into a silent no-op for the life of the queue.
        processing_ = true;
        execThread_ = std::this_thread::get_id();
        TaskQueueImpl::ProcessTasks(lock, ctime);
        execThread_ = std::thread::id();
        processing_ = false;
    }

private:
    // clang-format off
    uint64_t threadId_{0};
    // clang-format on
    // Guards against a task pumping the queue it runs on. Only read and written under mutex_.
    bool processing_{false};
};
// Internal api for engine task queue
namespace Internal {

IObjectFactory::Ptr GetPollingTaskQueueFactory()
{
    return PollingTaskQueue::GetFactory();
}

}  // namespace Internal

META_END_NAMESPACE()
