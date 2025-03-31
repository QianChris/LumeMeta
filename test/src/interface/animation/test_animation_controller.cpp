/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Animation Controller tests.
 * Author: Rafal Lazinski
 * Create: 2023-05-09
 */

#include <animation_test_base.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <meta/api/animation.h>
#include <meta/api/property/property_event_handler.h>
#include <meta/api/util.h>
#include <meta/base/namespace.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation_controller.h>
#include <meta/interface/property/construct_property.h>

#include "serialisation_utils.h"

using namespace testing;

META_BEGIN_NAMESPACE()

namespace Test {

class AnimationControllerTest : public AnimationTestBase {
protected:
    void SetUp() override
    {
        AnimationTestBase::SetUp();

        property_ = META_NS::ConstructProperty<float>("prop", 0.f);
        sutAnimationCtrl_ = META_NS::GetAnimationController();
        ASSERT_THAT(sutAnimationCtrl_, NotNull());
    }

    void TearDown() override
    {
        auto count = GetValue(sutAnimationCtrl_->Count());

        sutAnimationCtrl_->Clear();

        count = GetValue(sutAnimationCtrl_->Count());

        ASSERT_EQ(sutAnimationCtrl_->GetRunning().size(), 0);
        AnimationTestBase::TearDown();
    }

    META_NS::KeyframeAnimation<float> CreateAnimation()
    {
        auto animation = META_NS::KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation));
        animation.SetProperty(property_);
        animation.SetFrom(0.f);
        animation.SetTo(10.f);
        animation.SetDuration(ANIMATION_LENGTH);
        animation.SetController(sutAnimationCtrl_);
        return animation;
    }

    static constexpr auto ANIMATION_LENGTH = TimeSpan::Milliseconds(100);

    META_NS::Property<float> property_;
    META_NS::IAnimationController::Ptr sutAnimationCtrl_;
};

TEST_F(AnimationControllerTest, RunningAnimationListContainsStartedAnimations)
{
    // given
    auto anim = CreateAnimation();

    // when
    anim.Start();

    // expected
    ASSERT_EQ(anim.GetRunning(), true);
    const auto runningAnimations = sutAnimationCtrl_->GetRunning();
    ASSERT_THAT(runningAnimations.size(), Eq(1));
    EXPECT_THAT(runningAnimations[0].lock(), Eq(anim));
}

TEST_F(AnimationControllerTest, RunningAnimationListIsEmptyWhenAllAnimationsWereStopped)
{
    // given
    auto anim = CreateAnimation();

    // when
    anim.Start();
    anim.Stop();

    // expected
    ASSERT_EQ(anim.GetRunning(), false);
    EXPECT_TRUE(sutAnimationCtrl_->GetRunning().empty());
}

TEST_F(AnimationControllerTest, RunningAnimationListContainsResumedAnimations)
{
    // given
    auto anim = CreateAnimation();

    // when
    anim.Start();
    anim.Pause();
    anim.Start();

    // expected
    ASSERT_EQ(anim.GetRunning(), true);
    const auto runningAnimations = sutAnimationCtrl_->GetRunning();
    ASSERT_THAT(runningAnimations.size(), Eq(1));
    EXPECT_THAT(runningAnimations[0].lock(), Eq(anim));
}

TEST_F(AnimationControllerTest, RunningAnimationListIsEmptyWhenAllAnimationsWereFinished)
{
    // given
    auto anim = CreateAnimation();

    // when
    anim.Start();
    StepAnimationController(0);
    StepAnimationController(ANIMATION_LENGTH.ToMilliseconds());

    // expected
    ASSERT_EQ(anim.GetRunning(), false);
    ASSERT_EQ(anim.GetProgress(), 1.f);
    EXPECT_TRUE(sutAnimationCtrl_->GetRunning().empty());
}

TEST_F(AnimationControllerTest, AnimationControllerUpdatesControllerPropOfAddedAnimation)
{
    // given
    auto anim = CreateAnimation();
    anim.SetController({});
    ASSERT_TRUE(anim.GetController().expired());

    // when
    sutAnimationCtrl_->AddAnimation(anim);

    // expected
    ASSERT_FALSE(anim.GetController().expired());
    EXPECT_THAT(anim.GetController().lock(), Eq(sutAnimationCtrl_));
}

TEST_F(AnimationControllerTest, Serialization)
{
    TestSerialiser ser;
    static constexpr auto ImplicitPropertyName = "ImplicitProperty";
    static constexpr auto KeyframePropertyName = "KeyframeProperty";

    {
        auto controller = GetObjectRegistry().Create<IAnimationController>(META_NS::ClassId::AnimationController);

        auto implicitProp = ConstructProperty<float>(ImplicitPropertyName, 0);
        auto keyframeProp = ConstructProperty<float>(KeyframePropertyName, 0);

        Object object(CreateInstance(ClassId::Object));
        Metadata(object).AddProperty(implicitProp);
        Metadata(object).AddProperty(keyframeProp);

        auto implicitAnim = PropertyAnimation(CreateInstance(ClassId::PropertyAnimation))
                                .SetProperty(implicitProp)
                                .SetController(controller);
        auto keyframeAnim = SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
        keyframeAnim.SetController(controller);
        keyframeAnim.Add(KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation))
                             .SetFrom(0)
                             .SetTo(10)
                             .SetProperty(keyframeProp)
                             .SetDuration(TimeSpan::Milliseconds(100)));
        keyframeAnim.Add(KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation))
                             .SetFrom(10)
                             .SetTo(0)
                             .SetProperty(keyframeProp)
                             .SetDuration(TimeSpan::Milliseconds(100)));

        AttachmentContainer(object).Attach(implicitAnim);
        AttachmentContainer(object).Attach(keyframeAnim);
        AttachmentContainer(object).Attach(controller);

        ASSERT_TRUE(ser.Export(object));
        ser.Dump("project://animation_ser.ui");
    }

    auto obj = ser.Import();
    ASSERT_TRUE(obj);
    Object imported(obj);
    ASSERT_TRUE(imported);

    auto controllers =
        AttachmentContainer(imported).GetAttachments({ TypeId(META_NS::InterfaceId::IAnimationController) });
    ASSERT_EQ(controllers.size(), 1);

    auto importedController = interface_pointer_cast<IAnimationController>(controllers[0]);
    EXPECT_EQ(importedController->Count()->GetValue(), 2);

    auto animations = importedController->GetAnimations();
    ASSERT_EQ(animations.size(), 2);

    for (const auto& anim : animations) {
        auto animation = anim.lock();
        ASSERT_TRUE(animation);
        auto controller = animation->Controller()->GetValue().lock();
        EXPECT_EQ(controller, importedController);
    }

    // Check that we got the same animation hierarchy back
    PropertyAnimation implicitAnim(animations[0].lock());
    SequentialAnimation seqAnim(animations[1].lock());
    ASSERT_TRUE(implicitAnim);
    ASSERT_TRUE(seqAnim);
    auto children = seqAnim.GetAnimations();
    ASSERT_EQ(children.size(), 2);
    KeyframeAnimation<float> segment1(children[0]);
    ASSERT_TRUE(segment1);
    EXPECT_TRUE(segment1.GetValid());

    // Start the sequential animation and check that it works
    seqAnim.Start();
    importedController->Step(GetTestClock());

    EXPECT_TRUE(seqAnim.GetRunning());
    EXPECT_EQ(seqAnim.GetProgress(), 0);
    auto progress = seqAnim.GetProgress();

    for (auto i = 0; i < 5; i++) {
        IncrementClockTime(TimeSpan::Milliseconds(10));
        importedController->Step(GetTestClock());
        auto current = seqAnim.GetProgress();
        EXPECT_GT(current, progress);
        progress = current;
    }
}

} // namespace Test

META_END_NAMESPACE()
