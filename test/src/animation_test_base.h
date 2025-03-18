/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Animation test base
 * Author: Slawomir Grabowski
 * Create: 2022-07-20
 */

#ifndef META_TEST_ANIMATION_TEST_BASE_H
#define META_TEST_ANIMATION_TEST_BASE_H

#include <gtest/gtest.h>

#include <meta/base/time_span.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_manual_clock.h>

META_BEGIN_NAMESPACE()
namespace Test {

class AnimationTestBase : public ::testing::Test {
protected:
    void SetUp() override;
    /**
     * @brief Gives time stored in manual clock.
     * @return time of test clock.
     */
    TimeSpan Time() const;
    /**
     * @brief Increment clock time.
     *        Replacement for sleeping thread to avoid extending test application run time.
     * @param time Amount of time added to clock time.
     */
    void IncrementClockTime(const TimeSpan& time);

    void Update(const TimeSpan& time = TimeSpan::Milliseconds(0));

    /**
     * @brief Provides interface to test clock
     * @return Pointer to test clock
     */
    IClock::Ptr GetTestClock() const;
    /**
     * @brief Updates the view for a configurable number of frames.
     * @param frames number of frames to process.
     * @param updateFn Calls updateFn after each update.
     */
    void RunFrames(uint32_t frames, std::function<void(uint32_t frame)> updateFn);
    /**
     * @brief Steps the given list of animations over for a configurable number of frames.
     *        This function is useful for updating animations that are not part of a view
     *        hierarchy. For animations that animate widget properties, use RunFrames.
     * @param animations A list of animations to run.
     * @param frames Number of frames to process.
     * @param frameStepMs Number of ms to step between each frame
     * @param updateFn Calls updateFn after each step.
     */
    void StepAnimations(const std::vector<IAnimation::Ptr> animations, uint32_t frames, int64_t frameStepMs,
        std::function<void(uint32_t frame)> updateFn = {});
    /**
     * @brief Steps the given list of animations over for a configurable number of frames.
     *        This function is useful for updating animations that are not part of a view
     *        hierarchy. For animations that animate widget properties, use RunFrames.
     * @param animations A list of animations to run.
     * @param frames Number of frames to process.
     * @param updateFn Calls updateFn after each step.
     */
    void StepAnimations(
        const std::vector<IAnimation::Ptr> animations, uint32_t frames, std::function<void(uint32_t frame)> updateFn);
    /**
     * @brief Steps the global animation controller.
     * @param stepMs Milliseconds to step.
     */
    void StepAnimationController(int64_t stepMs = 10);
    /**
     * @brief Returns the animation controller used by the tests.
     */
    IAnimationController::Ptr GetAnimationController();

private:
    IManualClock::Ptr clock_;
};

} // namespace Test
META_END_NAMESPACE()

#endif // META_ANIMATION_TEST_BASE_H
