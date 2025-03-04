/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Easing curve user API
 * Author: Lauri Jaaskela
 * Create: 2021-10-05
 */

#ifndef META_API_EASING_CURVE_H
#define META_API_EASING_CURVE_H

#include <meta/api/internal/object_api.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/curves/intf_easing_curve.h>

META_BEGIN_NAMESPACE()

namespace Curves {
namespace Easing {

#define META_IMPLEMENT_EASING_CURVE_API(name)                                                     \
    class name : public Internal::ObjectInterfaceAPI<name, META_NS::ClassId::name##EasingCurve> { \
        META_API(name)                                                                            \
        META_API_OBJECT_CONVERTIBLE(META_NS::ICurve1D)                                            \
        META_API_OBJECT_CONVERTIBLE(META_NS::IEasingCurve)                                        \
    };

META_IMPLEMENT_EASING_CURVE_API(Linear)
META_IMPLEMENT_EASING_CURVE_API(InQuad)
META_IMPLEMENT_EASING_CURVE_API(OutQuad)
META_IMPLEMENT_EASING_CURVE_API(InOutQuad)
META_IMPLEMENT_EASING_CURVE_API(InCubic)
META_IMPLEMENT_EASING_CURVE_API(OutCubic)
META_IMPLEMENT_EASING_CURVE_API(InOutCubic)
META_IMPLEMENT_EASING_CURVE_API(InSine)
META_IMPLEMENT_EASING_CURVE_API(OutSine)
META_IMPLEMENT_EASING_CURVE_API(InOutSine)
META_IMPLEMENT_EASING_CURVE_API(InQuart)
META_IMPLEMENT_EASING_CURVE_API(OutQuart)
META_IMPLEMENT_EASING_CURVE_API(InOutQuart)
META_IMPLEMENT_EASING_CURVE_API(InQuint)
META_IMPLEMENT_EASING_CURVE_API(OutQuint)
META_IMPLEMENT_EASING_CURVE_API(InOutQuint)
META_IMPLEMENT_EASING_CURVE_API(InExpo)
META_IMPLEMENT_EASING_CURVE_API(OutExpo)
META_IMPLEMENT_EASING_CURVE_API(InOutExpo)
META_IMPLEMENT_EASING_CURVE_API(InCirc)
META_IMPLEMENT_EASING_CURVE_API(OutCirc)
META_IMPLEMENT_EASING_CURVE_API(InOutCirc)
META_IMPLEMENT_EASING_CURVE_API(InBack)
META_IMPLEMENT_EASING_CURVE_API(OutBack)
META_IMPLEMENT_EASING_CURVE_API(InOutBack)
META_IMPLEMENT_EASING_CURVE_API(InElastic)
META_IMPLEMENT_EASING_CURVE_API(OutElastic)
META_IMPLEMENT_EASING_CURVE_API(InOutElastic)
META_IMPLEMENT_EASING_CURVE_API(InBounce)
META_IMPLEMENT_EASING_CURVE_API(OutBounce)
META_IMPLEMENT_EASING_CURVE_API(InOutBounce)
META_IMPLEMENT_EASING_CURVE_API(StepStart)
META_IMPLEMENT_EASING_CURVE_API(StepEnd)
} // namespace Easing
} // namespace Curves

META_END_NAMESPACE()

#endif // META_API_EASING_CURVE_H
