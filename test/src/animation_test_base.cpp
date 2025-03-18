/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Animation test base implementation
 * Author: Slawomir Grabowski
 * Create: 2022-07-20
 */

#include "animation_test_base.h"

#include <gmock/gmock-matchers.h>

#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_registry.h>

using namespace testing;

META_BEGIN_NAMESPACE()
namespace Test {

void AnimationTestBase::SetUp()
{
    ::testing::Test::SetUp();
    clock_ = GetObjectRegistry().Create<IManualClock>(META_NS::ClassId::ManualClock);
}

TimeSpan AnimationTestBase::Time() const
{
    return clock_->GetTime();
}

void AnimationTestBase::IncrementClockTime(const TimeSpan& time)
{
    clock_->IncrementTime(time);
}

void AnimationTestBase::Update(const TimeSpan& time)
{
    IncrementClockTime(time);
    GetAnimationController()->Step(clock_);
}

IClock::Ptr AnimationTestBase::GetTestClock() const
{
    return clock_;
}

IAnimationController::Ptr AnimationTestBase::GetAnimationController()
{
    return META_NS::GetAnimationController();
}

void AnimationTestBase::RunFrames(uint32_t frames, std::function<void(uint32_t frame)> updateFn)
{
    // Sleep for a bit before first update to allow e.g. animations to have some time
    // to proceed before the first animation frame.
    IncrementClockTime(TimeSpan::Milliseconds(10));
    for (uint32_t i = 0; i < frames; i++) {
        GetAnimationController()->Step(GetTestClock());
        updateFn(i + 1);
        if (i < frames - 1)
            IncrementClockTime(TimeSpan::Milliseconds(10));
    }
}

void AnimationTestBase::StepAnimationController(int64_t stepMs)
{
    clock_->IncrementTime(TimeSpan::Milliseconds(stepMs));
    GetAnimationController()->Step(GetTestClock());
}

void AnimationTestBase::StepAnimations(
    const std::vector<IAnimation::Ptr> animations, uint32_t frames, std::function<void(uint32_t frame)> updateFn)
{
    StepAnimations(animations, frames, 10, updateFn);
}

void AnimationTestBase::StepAnimations(const std::vector<IAnimation::Ptr> animations, uint32_t frames,
    int64_t frameStepMs, std::function<void(uint32_t frame)> updateFn)
{
    IncrementClockTime(TimeSpan::Milliseconds(frameStepMs));
    for (uint32_t i = 0; i < frames; i++) {
        for (auto& animation : animations) {
            animation->Step(GetTestClock());
        }
        if (updateFn) {
            updateFn(i + 1);
        }
        if (i < frames - 1)
            IncrementClockTime(TimeSpan::Milliseconds(frameStepMs));
    }
}

} // namespace Test
META_END_NAMESPACE()
