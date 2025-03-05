/**
 *  Copyright (C) 2024 Huawei Technologies Co, Ltd.
 */
#include <animation_test_base.h>
#include <tester.h>
#include <testing_objects.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/api/animation/animation.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/property/construct_property.h>

using namespace testing;

META_BEGIN_NAMESPACE()

namespace Test {

class SequentialAnimationTest : public AnimationTestBase {
protected:
};

TEST_F(SequentialAnimationTest, SeekingSequentialAnimationNeedsToBeStartedToUpdateAnimatedPropertyValues)
{
    // given
    auto property1 = ConstructProperty("Property1", 0.0F);
    auto property2 = ConstructProperty("Property2", 0.0F);

    auto animation = SequentialAnimation()
                         .Child(KeyframeAnimation<float>().Property(property1).From(0.0F).To(1.0F).Duration(
                             TimeSpan::Milliseconds(1000)))
                         .Child(KeyframeAnimation<float>().Property(property2).From(0.0F).To(1.0F).Duration(
                             TimeSpan::Milliseconds(1000)));
    // when
    animation.Seek(0.25F);
    animation.Start();
    animation.Pause();
    auto progress25Property1Value = property1->GetValue();
    auto progress25Property2Value = property2->GetValue();

    animation.Seek(0.5F);
    animation.Start();
    animation.Pause();

    auto progress50Property1Value = property1->GetValue();
    auto progress50Property2Value = property2->GetValue();

    animation.Seek(0.75F);
    animation.Start();
    animation.Pause();

    auto progress75Property1Value = property1->GetValue();
    auto progress75Property2Value = property2->GetValue();

    // expected
    EXPECT_THAT(progress25Property1Value, Eq(0.5F));
    EXPECT_THAT(progress25Property2Value, Eq(0.0F));

    EXPECT_THAT(progress50Property1Value, Eq(1.0F));
    EXPECT_THAT(progress50Property2Value, Eq(0.0F));

    EXPECT_THAT(progress75Property1Value, Eq(1.0F));
    EXPECT_THAT(progress75Property2Value, Eq(0.5F));
}

TEST_F(SequentialAnimationTest, SeekingSequentialAnimationAndPause)
{
    // given
    auto property1 = ConstructProperty("Property1", 0.0F);
    auto property2 = ConstructProperty("Property2", 0.0F);

    auto animation = SequentialAnimation()
                         .Child(KeyframeAnimation<float>().Property(property1).From(0.0F).To(1.0F).Duration(
                             TimeSpan::Milliseconds(1000)))
                         .Child(KeyframeAnimation<float>().Property(property2).From(0.0F).To(1.0F).Duration(
                             TimeSpan::Milliseconds(1000)));

    // when
    animation.Start();
    animation.Seek(0.25F);
    animation.Pause();

    auto progress25Property1Value = property1->GetValue();
    auto progress25Property2Value = property2->GetValue();

    animation.Start();
    animation.Seek(0.5F);
    animation.Pause();

    auto progress50Property1Value = property1->GetValue();
    auto progress50Property2Value = property2->GetValue();

    animation.Start();
    animation.Seek(0.75F);
    animation.Pause();

    auto progress75Property1Value = property1->GetValue();
    auto progress75Property2Value = property2->GetValue();

    // expected
    EXPECT_THAT(progress25Property1Value, Eq(0.5F));
    EXPECT_THAT(progress25Property2Value, Eq(0.0F));

    EXPECT_THAT(progress50Property1Value, Eq(1.0F));
    EXPECT_THAT(progress50Property2Value, Eq(0.0F));

    EXPECT_THAT(progress75Property1Value, Eq(1.0F));
    EXPECT_THAT(progress75Property2Value, Eq(0.5F));
}

} // namespace Test
META_END_NAMESPACE()