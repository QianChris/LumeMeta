/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Animation Controller tests.
 * Author: Rafal Lazinski
 * Create: 2023-05-09
 */

#include <animation_test_base.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <meta/api/animation/animation.h>
#include <meta/base/namespace.h>
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
        auto animation = META_NS::KeyframeAnimation<float>()
                             .Property(property_)
                             .From(0.f)
                             .To(10.f)
                             .Duration(ANIMATION_LENGTH)
                             .Controller(sutAnimationCtrl_);
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
    ASSERT_EQ(GetValue(anim.Running()), true);
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
    ASSERT_EQ(GetValue(anim.Running()), false);
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
    ASSERT_EQ(GetValue(anim.Running()), true);
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
    ASSERT_EQ(GetValue(anim.Running()), false);
    ASSERT_EQ(GetValue(anim.Progress()), 1.f);
    EXPECT_TRUE(sutAnimationCtrl_->GetRunning().empty());
}

TEST_F(AnimationControllerTest, AnimationControllerUpdatesControllerPropOfAddedAnimation)
{
    // given
    auto anim = CreateAnimation();
    anim.Controller()->Reset();
    ASSERT_TRUE(GetValue(anim.Controller()).expired());

    // when
    sutAnimationCtrl_->AddAnimation(anim);

    // expected
    ASSERT_FALSE(GetValue(anim.Controller()).expired());
    EXPECT_THAT(GetValue(anim.Controller()).lock(), Eq(sutAnimationCtrl_));
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

        Object object;
        object.AddProperty(implicitProp);
        object.AddProperty(keyframeProp);

        auto implicitAnim = PropertyAnimation().Property(implicitProp).Controller(controller);
        auto keyframeAnim = SequentialAnimation()
                                .Controller(controller)
                                .Child(KeyframeAnimation<float>()
                                           .From(0)
                                           .To(10)
                                           .Duration(TimeSpan::Milliseconds(100))
                                           .Property(keyframeProp))
                                .Child(KeyframeAnimation<float>()
                                           .From(10)
                                           .To(0)
                                           .Duration(TimeSpan::Milliseconds(100))
                                           .Property(keyframeProp));

        object.Attach(interface_pointer_cast<IAttachment>(implicitAnim));
        object.Attach(interface_pointer_cast<IAttachment>(keyframeAnim));
        object.Attach(interface_pointer_cast<IAttachment>(controller));

        ASSERT_TRUE(ser.Export(object));
        ser.Dump("project://animation_ser.ui");
    }

    auto obj = ser.Import();
    ASSERT_TRUE(obj);
    Object imported;
    ASSERT_TRUE(imported.Initialize(obj));

    auto controllers = imported.GetAttachments({ TypeId(META_NS::InterfaceId::IAnimationController) });
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
    PropertyAnimation implicitAnim;
    SequentialAnimation seqAnim;
    KeyframeAnimation<float> segment1;
    ASSERT_TRUE(implicitAnim.Initialize(interface_pointer_cast<IObject>(animations[0])));
    ASSERT_TRUE(seqAnim.Initialize(interface_pointer_cast<IObject>(animations[1])));
    auto children = seqAnim.GetAnimations();
    ASSERT_EQ(children.size(), 2);
    ASSERT_TRUE(segment1.Initialize(interface_pointer_cast<IObject>(children[0])));
    EXPECT_TRUE(segment1.Valid()->GetValue());

    // Start the sequential animation and check that it works
    seqAnim.Start();
    importedController->Step(GetTestClock());

    EXPECT_TRUE(seqAnim.Running()->GetValue());
    EXPECT_EQ(seqAnim.Progress()->GetValue(), 0);
    auto progress = seqAnim.Progress()->GetValue();

    for (auto i = 0; i < 5; i++) {
        IncrementClockTime(TimeSpan::Milliseconds(10));
        importedController->Step(GetTestClock());
        auto current = seqAnim.Progress()->GetValue();
        EXPECT_GT(current, progress);
        progress = current;
    }
}

} // namespace Test

META_END_NAMESPACE()
