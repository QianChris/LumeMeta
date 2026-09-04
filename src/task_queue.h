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

#ifndef META_SRC_TASK_QUEUE_H
#define META_SRC_TASK_QUEUE_H

#include <chrono>
#include <deque>
#include <mutex>
#include <thread>

#include <base/containers/vector.h>
#include <core/log.h>

#include <meta/interface/intf_clock.h>
#include <meta/interface/intf_task_queue.h>

META_BEGIN_NAMESPACE()

class TaskQueueImpl : public ITaskQueueExtend {
public:
    using Token = ITaskQueue::Token;

    void SetExtend(ITaskQueueExtend* extend) override
    {
        extend_ = extend ? extend : this;
    }
    void Shutdown() override
    {
        Close();
    }

    void CancelTask(Token token)
    {
        // destroy tasks when mutex is not locked. A token can match more than once, the same
        // task can be queued several times and can sit in a batch and in the queue at once, so
        // this collects every match rather than holding one at a time.
        BASE_NS::vector<ITaskQueueTask::Ptr> removed;
        if (token != nullptr) {
            std::unique_lock lock{mutex_};
            if (token == execToken_) {
                // Currently executing task is requested to cancel.
                // Tasks are temporarily removed from the queue while execution, so the currently running task is not in
                // the queue any more. Setting currentlyExecutingRemoved to true will cause the task to not be re-added.
                currentlyExecutingRemoved = true;
            }

            // If we are currently executing the task in different thread, wait for it to complete.
            if (std::this_thread::get_id() != execThread_) {
                while (!terminate_ && token == execToken_) {
                    lock.unlock();
                    // sleep a bit.
                    std::this_thread::yield();
                    lock.lock();
                }
            }

            // Remove all tasks with the same token. (if any)
            // One can push the same task to the queue multiple times currently.
            // (i.e. you "can" schedule the same task with different "delays")
            // So we remove all scheduled tasks with same token.
            auto eraseToken = [token, &removed](auto& container) {
                for (auto it = container.begin(); it != container.end();) {
                    if (it->operation.get() == token) {
                        removed.push_back(BASE_NS::move(it->operation));
                        it = container.erase(it);
                    } else {
                        ++it;
                    }
                }
            };
            // A task can be waiting in any of the three: scheduled, held for the pass that is
            // running it, or on its way back after asking to repeat, which it can reach while we
            // yield above waiting for it to stop. Miss one and the cancel quietly does nothing,
            // so all three are swept.
            eraseToken(tasks_);
            eraseToken(running_);
            eraseToken(rearm_);
        }
    }

    Token AddTaskImpl(ITaskQueueTask::Ptr p, const TimeSpan& delay, const TimeSpan& excTime)
    {
        Token ret{p.get()};

        if (auto i = interface_cast<ITaskScheduleInfo>(p)) {
            i->SetQueueAndToken(self_.lock(), ret);
        }

        // Insertion sort, execute time descending; ProcessTasks takes from the back. Inserting
        // before the first task due at or before us keeps tasks that share a timestamp -- routine
        // at microsecond resolution -- in the order they were added.
        bool found = false;
        for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
            if (it->executeTime <= excTime) {
                // task in list should execute after us, so insert there.
                tasks_.insert(it, {delay, excTime, BASE_NS::move(p)});
                found = true;
                break;
            }
        }
        if (!found) {
            // Nothing runs at or before our time: we are the most due, and the back is next up.
            tasks_.emplace_back(delay, excTime, BASE_NS::move(p));
        }
        return ret;
    }

    Token AddTask(ITaskQueueTask::Ptr p, const TimeSpan& delay, const TimeSpan& excTime)
    {
        if (p) {
            std::unique_lock lock{mutex_};
            if (terminate_) {
                // The queue is closed: nothing processes tasks_ any more (a threaded queue has already
                // joined its worker). A task accepted here would sit in the queue forever, and anything
                // waiting on its future -- AddWaitableTask, RunDirectlyOrInTask -- would block forever
                // with no thread left to wake it. Reject so the caller can abandon the future instead.
                lock.unlock();
                CORE_LOG_D("Task added to a task queue that is shut down, discarding it");
                return nullptr;
            }
            return AddTaskImpl(BASE_NS::move(p), delay, excTime);
        }
        return nullptr;
    }

    TimeSpan Time() const
    {
        using namespace std::chrono;
        return TimeSpan::Microseconds(
            duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count());
    }

    // Runs the tasks that are due at curTime, oldest first.
    void ProcessTasks(std::unique_lock<std::mutex>& lock, TimeSpan curTime)
    {
        // Must only be called while having the lock.
        // Take what is due out of the queue before running any of it. That fixes the work this
        // call will do, so a task that queues more, itself included, cannot extend the pass.
        // Anything queued while we run goes to tasks_ and waits for the next call.
        //
        // tasks_ is ordered by executeTime descending, so the due tasks are the block at the back
        // and taking from the back gives running_ the oldest first.
        while (!terminate_ && !tasks_.empty() && (curTime >= tasks_.back().executeTime)) {
            running_.push_back(BASE_NS::move(tasks_.back()));
            tasks_.pop_back();
        }

        while (!terminate_ && !running_.empty()) {
            auto task = BASE_NS::move(running_.front());
            running_.pop_front();
            execToken_ = task.operation.get();
            currentlyExecutingRemoved = false;
            lock.unlock();
            bool redo = extend_->InvokeTask(task.operation);
            if (!redo) {
                // destroy before re-locking
                task.operation.reset();
            }
            lock.lock();
            if (currentlyExecutingRemoved) {
                // special case where the task was removed while still executing
                lock.unlock();
                task.operation.reset();
                lock.lock();
            } else if (redo) {
                // Reschedule the task again.
                rearm_.emplace_back(BASE_NS::move(task));
            }
            execToken_ = nullptr;
        }

        if (!terminate_) {
            // rearm the tasks..
            Rearm(curTime);
        } else {
            // Shutting down, so the repeats set aside for Rearm are dropped instead of returned.
            // Close cannot do it: it runs while the task is still executing, and the task only
            // reaches rearm_ after that. They are released with the lock dropped, because a
            // callback can re-enter the queue as it dies, CancelTask included, and that would
            // take this same mutex.
            auto abandoned = BASE_NS::move(rearm_);
            lock.unlock();
            abandoned.clear();
            lock.lock();
        }
    }

    void Rearm(TimeSpan curTime)
    {
        // In the order they ran: recurring tasks that come due together keep that order, because
        // AddTaskImpl puts equal timestamps back in addition order.
        for (auto it = rearm_.begin(); it != rearm_.end(); ++it) {
            auto& task = *it;
            if (task.delay > TimeSpan()) {
                // calculate the next executeTime in phase.. (ie. how many events missed)
                uint64_t dt = static_cast<uint64_t>(task.delay.ToMicroseconds());
                uint64_t et = static_cast<uint64_t>(task.executeTime.ToMicroseconds());
                uint64_t ct = static_cast<uint64_t>(curTime.ToMicroseconds());
                // calculate the next executeTime in phase..
                et += dt;
                if (et <= ct) {
                    // "ticks" how many events would have ran.. (rounded up)
                    auto ticks = ((ct - et) + (dt - 1)) / dt;
                    // and based on the "ticks" we can now count the next execution time.
                    et += (ticks * dt);
                    CORE_LOG_V("Skipped ticks %d", (int)ticks);
                }
                task.executeTime = TimeSpan::Microseconds(et);
            } else {
                task.executeTime = curTime;
            }
            AddTaskImpl(task.operation, task.delay, task.executeTime);
        }
        rearm_.clear();
    }

    void Close()
    {
        std::unique_lock lock{mutex_};
        terminate_ = true;
        tasks_.clear();
        // A pass interrupted by the shutdown leaves its batch here, so this has to go as well or
        // the work outlives the queue that was told to drop it.
        running_.clear();
    }

    struct Task {
        Task() = default;
        Task(TimeSpan d, TimeSpan e, const ITaskQueueTask::Ptr& p) : delay(d), executeTime(e), operation(p)
        {}

        TimeSpan delay;
        TimeSpan executeTime;
        ITaskQueueTask::Ptr operation{nullptr};
    };

protected:
    std::mutex mutex_;

    ITaskQueueExtend* extend_{this};
    bool terminate_{};
    std::thread::id execThread_;
    // currently running task..
    Token execToken_{nullptr};
    std::deque<Task> tasks_;
    // The batch the running pass took out of tasks_, oldest first. Separate so that work queued
    // while the pass runs cannot join it.
    std::deque<Task> running_;
    BASE_NS::vector<Task> rearm_;
    ITaskQueue::WeakPtr self_;
    bool currentlyExecutingRemoved{};
};

META_END_NAMESPACE()

#endif
