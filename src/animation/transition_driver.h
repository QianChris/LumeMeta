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

#ifndef META_SRC_ANIMATION_TRANSITION_DRIVER_H
#define META_SRC_ANIMATION_TRANSITION_DRIVER_H

#include <meta/interface/animation/transition.h>

#include <mutex>

#include <base/containers/vector.h>

#include <meta/base/time_span.h>
#include <meta/ext/object.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/animation/intf_interpolator.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_CLASS(TransitionDriver, "1872ec45-0837-4d50-8838-73da2542d276", META_NS::ObjectCategoryBits::NO_CATEGORY)

/**
 * @brief What a caller asks of a transition driver.
 *
 * Separate from the class because the driver is built through the object registry, which hands back an
 * interface rather than the type.
 */
class ITransitionDriver : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITransitionDriver, "a8ac8800-e633-4cd6-b64a-35412a0cdf44")
public:
    /**
     * @brief Move @p target towards @p to over @p transition instead of writing it at once, replacing any
     *        transition already running on that property. Returns false when the change cannot be
     *        interpolated — a zero duration, or a type with no interpolator — and the caller should write
     *        the value directly; a caller that gets false has had nothing done to it.
     *
     *        Retargets rather than restarts: a change arriving mid-flight moves on from where the value
     *        currently sits, because restarting from the value it was originally leaving snaps backwards.
     *
     * @param toDefault The property is returning to its default, so it is reset when the transition lands
     *                  rather than being left holding a value that merely equals the default.
     */
    virtual bool Start(const META_NS::IProperty::Ptr& target, const META_NS::IAny& to, const TransitionSpec& transition,
        bool toDefault) = 0;

    /**
     * @brief Drop any transition running on @p target, leaving it wherever it had reached. For a property
     *        that is going away or being replaced.
     */
    virtual void Cancel(const META_NS::IProperty& target) = 0;

    /**
     * @brief The queue the eased properties belong to, so a step lands on their thread rather than on
     *        whichever one the animation controller happens to run. Null means write where stepped.
     */
    virtual void SetQueue(const META_NS::ITaskQueue::Ptr& queue) = 0;
};

/**
 * @brief Moves properties towards target values over time.
 *
 * Nothing here is specific to what the properties are for: the alias registry owns one to ease its
 * cells, but a driver is a general answer to "blend these properties towards those values, cheaply".
 *
 * A transition is rare and short next to the number of properties a caller may hold, so rather than an
 * animation object per property this holds one flat row per property actually in flight and drops the
 * row when it lands. A three-hundred-value theme with twelve changed colours is twelve rows, and a
 * caller with nothing in flight holds none. That is the difference from META_NS::PropertyAnimation used
 * as an implicit animation: one object per property, and no way to animate a reset to the default,
 * since ResetValue does not pass through a stack's modifiers.
 *
 * It is an IAnimation only so that whatever already steps animations steps transitions too, and it
 * reports Running exactly while it holds rows, which is what the animation controller uses to decide
 * whether to step it. Nothing starts, seeks, curves or serializes it as a whole: a driver is a set of
 * independent rows rather than one timeline, so Progress, TotalDuration and Curve describe nothing and
 * are left at their defaults rather than given a made-up meaning.
 */
class TransitionDriver
    : public META_NS::IntroduceInterfaces<META_NS::MetaObject, META_NS::IAnimation, ITransitionDriver> {
    META_OBJECT(TransitionDriver, ClassId::TransitionDriver, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::IAnimation, bool, Enabled, true)
    META_STATIC_PROPERTY_DATA(META_NS::IAnimation, bool, Valid, true)
    META_STATIC_PROPERTY_DATA(META_NS::IAnimation, META_NS::TimeSpan, TotalDuration)
    META_STATIC_PROPERTY_DATA(META_NS::IAnimation, bool, Running)
    META_STATIC_PROPERTY_DATA(META_NS::IAnimation, float, Progress)
    META_STATIC_PROPERTY_DATA(META_NS::IAnimation, META_NS::ICurve1D::Ptr, Curve)
    META_STATIC_PROPERTY_DATA(META_NS::IAnimation, BASE_NS::weak_ptr<META_NS::IAnimationController>, Controller)
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_STATIC_EVENT_DATA(META_NS::IAnimation, META_NS::IOnChanged, OnFinished)
    META_STATIC_EVENT_DATA(META_NS::IAnimation, META_NS::IOnChanged, OnStarted)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_READONLY_PROPERTY(bool, Valid)
    META_IMPLEMENT_READONLY_PROPERTY(META_NS::TimeSpan, TotalDuration)
    META_IMPLEMENT_READONLY_PROPERTY(bool, Running)
    META_IMPLEMENT_READONLY_PROPERTY(float, Progress)
    META_IMPLEMENT_PROPERTY(META_NS::ICurve1D::Ptr, Curve)
    META_IMPLEMENT_PROPERTY(BASE_NS::weak_ptr<META_NS::IAnimationController>, Controller)
    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)
    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnFinished)
    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnStarted)

    // IAnimation
    void Step(const META_NS::IClock::ConstPtr& clock) override;

    // ITransitionDriver
    bool Start(const META_NS::IProperty::Ptr& target, const META_NS::IAny& to, const TransitionSpec& transition,
        bool toDefault) override;
    void Cancel(const META_NS::IProperty& target) override;
    void SetQueue(const META_NS::ITaskQueue::Ptr& queue) override;

private:
    // One property in flight. `from` and `to` are owned copies: the value a transition is leaving has no
    // other home once the property starts moving, and the one it is heading for may belong to something
    // released while the transition is still running.
    struct Row {
        META_NS::IProperty::Ptr target;
        META_NS::IAny::Ptr from;
        META_NS::IAny::Ptr to;
        META_NS::IInterpolator::Ptr interpolator;
        META_NS::ICurve1D::Ptr curve;
        META_NS::TimeSpan duration;
        META_NS::TimeSpan elapsed;
        bool toDefault{};
    };

    // Write the value the row has reached into its property. A row that has run its duration lands on
    // its target, and one returning to the default resets there rather than leaving a value that merely
    // equals the default, so that a later change of default still reaches the property.
    static void Apply(const Row& row);

    // Rows advance on whichever thread steps the animation controller while callers start them from
    // their own; this guards the rows and the step bookkeeping below, never a property write. Properties
    // are written with the lock released, because a change notification does arbitrary work — an
    // alias-driven node instantiating a whole subtree lands right back here.
    mutable std::mutex mutex_;
    BASE_NS::vector<Row> rows_;
    // The clock reading the last step saw, so a row advances by elapsed wall time rather than by a frame
    // count. `stepped_` says whether it means anything yet: a driver that has been idle must not charge a
    // row for the time it spent empty, and a clock reading zero is a reading like any other.
    META_NS::TimeSpan last_;
    bool stepped_{};
    bool running_{};
    // The queue the eased properties belong to. Set once by whoever owns the driver, before it runs.
    META_NS::ITaskQueue::WeakPtr queue_;
};

META_END_NAMESPACE()

#endif
