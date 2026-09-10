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

#include "transition_driver.h"

#include <meta/api/util.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation_controller.h>
#include <meta/interface/intf_clock.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/property.h>

#include "../run_on_queue.h"

META_BEGIN_NAMESPACE()

void TransitionDriver::SetQueue(const META_NS::ITaskQueue::Ptr& queue)
{
    queue_ = queue;
}

void TransitionDriver::Apply(const Row& row)
{
    const auto total = row.duration.ToSecondsFloat();
    float t = total > 0.f ? row.elapsed.ToSecondsFloat() / total : 1.f;
    if (t >= 1.f) {
        if (row.toDefault) {
            row.target->ResetValue();
        } else {
            META_NS::TypelessPropertyInterface{row.target.get()}.SetValueAny(*row.to);
        }
        return;
    }
    t = t < 0.f ? 0.f : t;
    const float eased = row.curve ? row.curve->Transform(t) : t;
    // Interpolated into a value of the target's own type, so a property whose type narrowed since the
    // row was built cannot be written with the wrong shape.
    auto value = row.to->Clone(false);
    if (value && row.interpolator->Interpolate(*value, *row.from, *row.to, eased)) {
        META_NS::TypelessPropertyInterface{row.target.get()}.SetValueAny(*value);
    }
}

bool TransitionDriver::Start(
    const META_NS::IProperty::Ptr& target, const META_NS::IAny& to, const TransitionSpec& transition, bool toDefault)
{
    if (!target || transition.duration <= META_NS::TimeSpan::Zero()) {
        return false;
    }
    auto interpolator = META_NS::GetObjectRegistry().CreateInterpolator(target);
    // A type with no interpolator of its own gets the default one, which does not blend: it holds the
    // starting value and steps to the target at the end. Down that road a material with a half-second
    // transition simply shows the old material for half a second, so refuse the row instead and let the
    // caller write the value outright.
    auto obj = interface_cast<META_NS::IObject>(interpolator);
    if (!interpolator || !obj || obj->GetClassId() == META_NS::ClassId::DefaultInterpolator) {
        return false;
    }
    Row row;
    row.target = target;
    row.from = target->GetValue().Clone(true);
    row.to = to.Clone(true);
    row.interpolator = BASE_NS::move(interpolator);
    row.curve = transition.curve;
    row.duration = transition.duration;
    row.toDefault = toDefault;
    if (!row.from || !row.to) {
        return false;
    }
    bool start = false;
    {
        std::unique_lock guard{mutex_};
        auto existing = rows_.begin();
        for (; existing != rows_.end(); ++existing) {
            if (existing->target == target) {
                break;
            }
        }
        if (existing != rows_.end()) {
            // Retarget: the row keeps its place and starts again from wherever the value now sits, which
            // `from` already captured. Restarting from the value it was originally leaving would snap
            // backwards on every change that arrives mid-flight.
            *existing = BASE_NS::move(row);
        } else {
            rows_.push_back(BASE_NS::move(row));
            start = !running_;
            running_ = true;
            if (start) {
                // A run that is starting must not be charged for the time the driver spent idle, and
                // `last_` is read by Step on whatever thread the animation controller runs, so this
                // belongs under the same lock as the rows it applies to.
                stepped_ = false;
            }
        }
    }
    if (start) {
        // Only now is there anything to step. The controller holds animations weakly and skips those not
        // running, so a registry whose transitions have all landed costs it a flag test.
        if (auto controller = META_NS::GetAnimationController()) {
            controller->AddAnimation(GetSelf<META_NS::IAnimation>());
        }
        META_NS::SetValue(META_ACCESS_PROPERTY(Running), true);
    }
    return true;
}

void TransitionDriver::Cancel(const META_NS::IProperty& target)
{
    bool stop = false;
    {
        std::unique_lock guard{mutex_};
        for (auto it = rows_.begin(); it != rows_.end(); ++it) {
            if (it->target.get() == &target) {
                rows_.erase(it);
                break;
            }
        }
        stop = rows_.empty() && running_;
        running_ = !rows_.empty();
    }
    if (stop) {
        META_NS::SetValue(META_ACCESS_PROPERTY(Running), false);
    }
}

void TransitionDriver::Step(const META_NS::IClock::ConstPtr& clock)
{
    if (!clock) {
        return;
    }
    const auto now = clock->GetTime();
    BASE_NS::vector<Row> due;
    bool stop = false;
    {
        std::unique_lock guard{mutex_};
        // The first step of a run charges nothing: `last_` only means something once a step has been seen
        // while these rows were in flight, and a driver that sat idle must not bill them for that time.
        const auto delta = stepped_ ? now - last_ : META_NS::TimeSpan::Zero();
        last_ = now;
        stepped_ = true;
        due.reserve(rows_.size());
        for (auto it = rows_.begin(); it != rows_.end();) {
            it->elapsed += delta;
            due.push_back(*it);
            if (it->elapsed >= it->duration) {
                it = rows_.erase(it);
            } else {
                ++it;
            }
        }
        stop = rows_.empty() && running_;
        running_ = !rows_.empty();
    }
    // Outside the lock: writing a property notifies, and a change handler does arbitrary work that can
    // land straight back in Start. On the properties' own queue when there is one, since a step arrives
    // on whatever thread the animation controller runs — a whole step at a time, not a task per row.
    RunOnQueueAndWait(queue_.lock(), [&due] {
        for (auto& row : due) {
            Apply(row);
        }
    });
    if (stop) {
        META_NS::SetValue(META_ACCESS_PROPERTY(Running), false);
    }
}

META_END_NAMESPACE()
