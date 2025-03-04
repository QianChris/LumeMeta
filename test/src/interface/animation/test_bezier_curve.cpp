/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Cubic bezier curve tests
 * Author: Lauri Jaaskela
 * Create: 2023-08-08
 */

#include <animation_test_base.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <meta/api/animation/animation.h>
#include <meta/api/curves/cubic_bezier_easing_curve.h>
#include <meta/base/namespace.h>

using namespace testing;
using namespace META_NS::TimeSpanLiterals;

META_BEGIN_NAMESPACE()

namespace Test {

class BezierCurveTest : public AnimationTestBase {
protected:
    void SetUp() override
    {
        AnimationTestBase::SetUp();
        curve_ = interface_cast<ICurve1D>(bezier_);
        ASSERT_NE(curve_, nullptr);
        ASSERT_NE(interface_cast<IEasingCurve>(bezier_), nullptr);
        ASSERT_NE(interface_cast<ICubicBezier>(bezier_), nullptr);
    }

    void SetControlPoints()
    {
        // Set control points to create a steeply rising curve
        bezier_.ControlPoint1({ 1.f, 0.f }).ControlPoint2({ 0.0f, 1.f });
    }

    META_NS::ICurve1D* curve_ {};
    META_NS::Curves::Easing::CubicBezierEasingCurve bezier_;

    static constexpr float epsilon_ = 0.001f;
    static constexpr auto frames_ = 20;
    // Pre-calculated 20 cubic bezier curve values for cp1=[1.f,0.f] and cp2=[0.f, 1.f]
    static constexpr float expected_[] = { 0.000000, 0.000882, 0.003711, 0.009070, 0.017385, 0.029723, 0.047319,
        0.072904, 0.111333, 0.176019, 0.500000, 0.823981, 0.888667, 0.927096, 0.952681, 0.970277, 0.982615, 0.990930,
        0.996289, 0.999118, 1.000000 };
};

TEST_F(BezierCurveTest, BezierDefault)
{
    // Linear as control points are at default [0,0], [1,1]
    for (int i = 0; i < 100; i++) {
        float f = i / 100.f;
        EXPECT_NEAR(curve_->Transform(f), f, epsilon_);
    }
}

TEST_F(BezierCurveTest, BezierCurve)
{
    SetControlPoints();

    for (int i = 0; i <= frames_; i++) {
        float f = i / static_cast<float>(frames_);
        EXPECT_NEAR(curve_->Transform(f), expected_[i], epsilon_);
    }
}

TEST_F(BezierCurveTest, BezierAnimation)
{
    constexpr auto animationMs = 1000;
    constexpr float frameMs = animationMs / static_cast<float>(frames_);

    // Animate from 13.f->47.f
    constexpr auto start = 13.f;
    constexpr auto end = 47.f;
    constexpr auto diff = end - start;

    SetControlPoints();

    // Check that if we assign a bezier to an animation, the animated value follows the bezier as expected.
    auto property = META_NS::ConstructProperty<float>("Prop", 0.f);
    auto animation =
        META_NS::KeyframeAnimation<float>().Duration(1_s).Property(property).From(start).To(end).Curve(bezier_);

    animation.Start();
    StepAnimations({ animation }, frames_, frameMs, [&](uint32_t frame) {
        auto expected = start + expected_[frame - 1] * diff;
        EXPECT_NEAR(GetValue(property), expected, 0.1f) << "Frame: " << frame;
    });
}

} // namespace Test

META_END_NAMESPACE()
