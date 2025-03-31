/**
 *  Copyright (C) 2024 Huawei Technologies Co, Ltd.
 */
#include <animation_test_base.h>
#include <tester.h>
#include <testing_objects.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/api/animation.h>
#include <meta/api/object.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/property/construct_property.h>

using namespace testing;

META_BEGIN_NAMESPACE()

namespace Test {

namespace {

template<typename AnimationType>
Animation CreateAnimation(const IProperty::WeakPtr& property);

template<>
Animation CreateAnimation<SequentialAnimation>(const IProperty::WeakPtr& property)
{
    return SequentialAnimation(CreateInstance(ClassId::SequentialAnimation))
        .Add(KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation)));
}

template<>
Animation CreateAnimation<ParallelAnimation>(const IProperty::WeakPtr& property)
{
    return ParallelAnimation(CreateInstance(ClassId::ParallelAnimation))
        .Add(KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation)));
}

template<>
Animation CreateAnimation<KeyframeAnimation<float>>(const IProperty::WeakPtr& property)
{
    KeyframeAnimation<float> animation(CreateInstance(ClassId::KeyframeAnimation));
    animation.SetFrom(0.0F);
    animation.SetTo(1.0F);
    animation.SetDuration(TimeSpan::Milliseconds(1000));
    animation.SetProperty(property);
    return animation;
}

} // namespace

template<typename AnimationType>
class AnimationSeekingTest : public AnimationTestBase {
public:
    AnimationSeekingTest()
    {
        property_ = ConstructProperty("animatedProperty", 0.0F);
        startableInterface_ = interface_pointer_cast<IStartableAnimation>(CreateAnimation<AnimationType>(property_));
        animationInterface_ = interface_pointer_cast<IAnimation>(startableInterface_);
    }

protected:
    IStartableAnimation::Ptr startableInterface_;
    IAnimation::Ptr animationInterface_;
    Property<float> property_;
};

using TestedTypes = ::testing::Types<KeyframeAnimation<float>, SequentialAnimation, ParallelAnimation>;
TYPED_TEST_SUITE(AnimationSeekingTest, TestedTypes);

TYPED_TEST(AnimationSeekingTest, SeekingSequentialAnimationDoesNotCauseAnimationStart)
{
    // given
    auto invoked = false;
    this->animationInterface_->Running().GetProperty()->OnChanged()->AddHandler(
        MakeCallback<IOnChanged>([&] { invoked = true; }));

    // when
    this->startableInterface_->Seek(0.25F);

    // expected
    const auto isRunning = this->animationInterface_->Running()->GetValue();
    EXPECT_THAT(isRunning, Eq(false));
    EXPECT_THAT(invoked, Eq(false));
}

TYPED_TEST(AnimationSeekingTest, SeekingRunningSequentialAnimationDoesNotChangeState)
{
    // given
    this->startableInterface_->Start();

    auto invoked = false;
    this->animationInterface_->Running().GetProperty()->OnChanged()->AddHandler(
        MakeCallback<IOnChanged>([&] { invoked = true; }));

    // when
    this->startableInterface_->Seek(0.25F);

    // expected
    const auto isRunning = this->animationInterface_->Running()->GetValue();
    EXPECT_THAT(isRunning, Eq(true));
    EXPECT_THAT(invoked, Eq(false));
}

TYPED_TEST(AnimationSeekingTest, SeekingTo1000ProgressFinishesAnimation)
{
    // given
    this->startableInterface_->Start();

    auto animationRunningChangeInvoke = false;
    this->animationInterface_->Running().GetProperty()->OnChanged()->AddHandler(
        MakeCallback<IOnChanged>([&] { animationRunningChangeInvoke = true; }));

    auto onFinishedInvoked = false;
    this->animationInterface_->OnFinished()->AddHandler(MakeCallback<IOnChanged>([&] { onFinishedInvoked = true; }));

    // when
    this->startableInterface_->Seek(1.0F);

    // expected
    const auto isRunning = this->animationInterface_->Running()->GetValue();
    EXPECT_THAT(isRunning, Eq(false));
    EXPECT_THAT(animationRunningChangeInvoke, Eq(true));
    EXPECT_THAT(onFinishedInvoked, Eq(true));
}

TYPED_TEST(AnimationSeekingTest, SeekingToBeginningDoesNotStopAnimation)
{
    // given
    this->startableInterface_->Start();
    this->Update(TimeSpan::Milliseconds(500));

    // when
    auto animationRunningChangeInvoke = false;
    this->animationInterface_->Running().GetProperty()->OnChanged()->AddHandler(
        MakeCallback<IOnChanged>([&] { animationRunningChangeInvoke = true; }));

    this->startableInterface_->Seek(0.0F);

    // expected
    auto propertyValue = this->property_->GetValue();

    EXPECT_THAT(propertyValue, Eq(0.0F));

    const auto isRunning = this->animationInterface_->Running()->GetValue();
    EXPECT_THAT(isRunning, Eq(true));
    EXPECT_THAT(animationRunningChangeInvoke, Eq(false));
}

} // namespace Test
META_END_NAMESPACE()
