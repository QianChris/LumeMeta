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
#include "easing_curve.h"

#include <base/math/mathf.h>

META_BEGIN_NAMESPACE()

namespace Curves {
namespace Easing {

bool AreEqual(float v1, float v2)
{
    return BASE_NS::Math::abs(v1 - v2) < BASE_NS::Math::EPSILON;
}
float Pow2(float v)
{
    return v * v;
}
float Pow3(float v)
{
    return v * v * v;
}
float Pow4(float v)
{
    return v * v * v * v;
}
float Pow5(float v)
{
    return v * v * v * v * v;
}
float EaseLinear(float t)
{
    return t;
}
float EaseInSine(float t)
{
    return 1.f - BASE_NS::Math::cos((t * BASE_NS::Math::PI) / 2.f);
}
float EaseOutSine(float t)
{
    return BASE_NS::Math::sin((t * BASE_NS::Math::PI) / 2.f);
}
float EaseInOutSine(float t)
{
    return -(BASE_NS::Math::cos(BASE_NS::Math::PI * t) - 1.f) / 2.f;
}
float EaseInQuad(float t)
{
    return Pow2(t);
}
float EaseOutQuad(float t)
{
    return 1.f - Pow2(1.f - t);
}
float EaseInOutQuad(float t)
{
    if (t < 0.5f) {
        return 2 * t * t;
    }
    return 1.f - (Pow2(-2.f * t + 2.f) / 2.f);
}
float EaseInCubic(float t)
{
    return Pow3(t);
}
float EaseOutCubic(float t)
{
    return 1.f - Pow3(1.f - t);
}
float EaseInOutCubic(float t)
{
    if (t < 0.5f) {
        return 4 * t * t * t;
    }
    return 1.f - (Pow3(-2.f * t + 2.f) / 2.f);
}
float EaseInQuart(float t)
{
    return Pow4(t);
}
float EaseOutQuart(float t)
{
    return 1.f - Pow4(1.f - t);
}
float EaseInOutQuart(float t)
{
    if (t < 0.5f) {
        return 8 * Pow4(t);
    }
    return 1.f - (Pow4(-2.f * t + 2.f) / 2.f);
}
float EaseInQuint(float t)
{
    return Pow5(t);
}
float EaseOutQuint(float t)
{
    return 1.f - Pow5(1.f - t);
}
float EaseInOutQuint(float t)
{
    if (t < 0.5f) {
        return 16 * Pow5(t);
    }
    return 1.f - (Pow5(-2.f * t + 2.f) / 2.f);
}
float EaseInExpo(float t)
{
    if (AreEqual(t, 0.f)) {
        return 0;
    }
    return BASE_NS::Math::pow(2.f, 10 * t - 10);
}
float EaseOutExpo(float t)
{
    if (AreEqual(t, 1.f)) {
        return 1;
    }
    return 1.f - BASE_NS::Math::pow(2.f, -10 * t);
}
float EaseInOutExpo(float t)
{
    if (AreEqual(t, 0.f)) {
        return 0.f;
    }
    if (AreEqual(t, 1.f)) {
        return 1.f;
    }
    if (t < .5f) {
        return BASE_NS::Math::pow(2.f, 20 * t - 10) / 2.f;
    }
    return (2.f - BASE_NS::Math::pow(2.f, -20 * t + 10)) / 2.f;
}
float EaseInCirc(float t)
{
    return 1.f - BASE_NS::Math::sqrt(1.f - Pow2(t));
}
float EaseOutCirc(float t)
{
    return BASE_NS::Math::sqrt(1.f - Pow2(t - 1.f));
}
float EaseInOutCirc(float t)
{
    if (t < 0.5f) {
        return (1.f - BASE_NS::Math::sqrt(1.f - Pow2(2.f * t))) / 2.f;
    }
    return (BASE_NS::Math::sqrt(1.f - Pow2(-2.f * t + 2.f)) + 1.f) / 2.f;
}
float EaseInBack(float t)
{
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.f;
    return c3 * Pow3(t) - c1 * Pow2(t);
}
float EaseOutBack(float t)
{
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.f;
    return 1.f + c3 * Pow3(t - 1.f) + c1 * Pow2(t - 1.f);
}
float EaseInOutBack(float t)
{
    constexpr float c1 = 1.70158f;
    constexpr float c2 = c1 * 1.525f;
    if (t < 0.5f) {
        return (Pow2(2.f * t) * ((c2 + 1.f) * 2.f * t - c2)) / 2.f;
    }
    return (Pow2(2.f * t - 2.f) * ((c2 + 1.f) * (t * 2.f - 2.f) + c2) + 2.f) / 2.f;
}
float EaseInElastic(float t)
{
    constexpr float c4 = (2.f * BASE_NS::Math::PI) / 3.f;
    if (AreEqual(t, 0.f)) {
        return 0;
    }
    if (AreEqual(t, 1.f)) {
        return 1;
    }
    return -BASE_NS::Math::pow(2.f, 10 * t - 10) * BASE_NS::Math::sin((t * 10 - 10.75f) * c4);
}
float EaseOutElastic(float t)
{
    constexpr float c4 = (2.f * BASE_NS::Math::PI) / 3.f;
    if (AreEqual(t, 0.f)) {
        return 0;
    }
    if (AreEqual(t, 1.f)) {
        return 1;
    }
    return BASE_NS::Math::pow(2.f, -10 * t) * BASE_NS::Math::sin((t * 10 - 0.75f) * c4) + 1.f;
}
float EaseInOutElastic(float t)
{
    constexpr float c5 = (2.f * BASE_NS::Math::PI) / 4.5f;
    if (AreEqual(t, 0.f)) {
        return 0;
    }
    if (AreEqual(t, 1.f)) {
        return 1;
    }
    if (t < 0.5f) {
        return -(BASE_NS::Math::pow(2.f, 20 * t - 10) * BASE_NS::Math::sin((20 * t - 11.125f) * c5)) / 2.f;
    }
    return (BASE_NS::Math::pow(2.f, -20 * t + 10) * BASE_NS::Math::sin((20 * t - 11.125f) * c5)) / 2.f + 1.f;
}
float EaseOutBounce(float t)
{
    constexpr float n1 = 7.5625f;
    constexpr float d1 = 2.75f;
    if (t < 1.f / d1) {
        return n1 * Pow2(t);
    }
    if (t < (2.f / d1)) {
        return n1 * Pow2(t - (1.5f / d1)) + 0.75f;
    }
    if (t < (2.5f / d1)) {
        return n1 * Pow2(t - (2.25f / d1)) + 0.9375f;
    }
    return n1 * Pow2(t - (2.625f / d1)) + 0.984375f;
}
float EaseInBounce(float t)
{
    return 1.f - EaseOutBounce(1.f - t);
}
float EaseInOutBounce(float t)
{
    if (t < 0.5f) {
        return (1.f - EaseOutBounce(1.f - 2.f * t)) / 2.f;
    }
    return (1.f + EaseOutBounce(2.f * t - 1.f)) / 2.f;
}
float EaseStepStart(float)
{
    return 1.f;
}
float EaseStepEnd(float t)
{
    return t < 1.f ? 0.f : 1.f;
}

// One data-driven EasingCurve factory per built-in curve: ClassId -> transform function.
#define EASING_CURVE_ENTRY(name)               \
    {                                          \
        ClassId::name##EasingCurve, Ease##name \
    }
static EasingCurveFactory g_easingCurveFactories[] = {
    EASING_CURVE_ENTRY(Linear),
    EASING_CURVE_ENTRY(InQuad),
    EASING_CURVE_ENTRY(OutQuad),
    EASING_CURVE_ENTRY(InOutQuad),
    EASING_CURVE_ENTRY(InCubic),
    EASING_CURVE_ENTRY(OutCubic),
    EASING_CURVE_ENTRY(InOutCubic),
    EASING_CURVE_ENTRY(InSine),
    EASING_CURVE_ENTRY(OutSine),
    EASING_CURVE_ENTRY(InOutSine),
    EASING_CURVE_ENTRY(InQuart),
    EASING_CURVE_ENTRY(OutQuart),
    EASING_CURVE_ENTRY(InOutQuart),
    EASING_CURVE_ENTRY(InQuint),
    EASING_CURVE_ENTRY(OutQuint),
    EASING_CURVE_ENTRY(InOutQuint),
    EASING_CURVE_ENTRY(InExpo),
    EASING_CURVE_ENTRY(OutExpo),
    EASING_CURVE_ENTRY(InOutExpo),
    EASING_CURVE_ENTRY(InCirc),
    EASING_CURVE_ENTRY(OutCirc),
    EASING_CURVE_ENTRY(InOutCirc),
    EASING_CURVE_ENTRY(InBack),
    EASING_CURVE_ENTRY(OutBack),
    EASING_CURVE_ENTRY(InOutBack),
    EASING_CURVE_ENTRY(InElastic),
    EASING_CURVE_ENTRY(OutElastic),
    EASING_CURVE_ENTRY(InOutElastic),
    EASING_CURVE_ENTRY(InBounce),
    EASING_CURVE_ENTRY(OutBounce),
    EASING_CURVE_ENTRY(InOutBounce),
    EASING_CURVE_ENTRY(StepStart),
    EASING_CURVE_ENTRY(StepEnd),
};
#undef EASING_CURVE_ENTRY

void RegisterEasingCurves(IObjectRegistry& registry)
{
    for (auto& f : g_easingCurveFactories) {
        registry.RegisterObjectType(IObjectFactory::Ptr{&f});
    }
}
void UnregisterEasingCurves(IObjectRegistry& registry)
{
    for (auto& f : g_easingCurveFactories) {
        registry.UnregisterObjectType(IObjectFactory::Ptr{&f});
    }
}

}  // namespace Easing
}  // namespace Curves

META_END_NAMESPACE()
