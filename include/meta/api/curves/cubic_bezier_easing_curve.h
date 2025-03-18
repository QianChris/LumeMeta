/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Cubic bezier easing curve high-level API
 * Author: Lauri Jaaskela
 * Create: 2023-08-08
 */

#ifndef META_API_CUBIC_BEZIER_EASING_CURVE_H
#define META_API_CUBIC_BEZIER_EASING_CURVE_H

#include <meta/api/internal/object_api.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/curves/intf_bezier.h>
#include <meta/interface/curves/intf_easing_curve.h>

META_BEGIN_NAMESPACE()

namespace Curves {
namespace Easing {

class CubicBezierEasingCurve
    : public Internal::ObjectInterfaceAPI<CubicBezierEasingCurve, META_NS::ClassId::CubicBezierEasingCurve> {
    META_API(CubicBezierEasingCurve)
    META_API_OBJECT_CONVERTIBLE(META_NS::ICurve1D)
    META_API_OBJECT_CONVERTIBLE(META_NS::IEasingCurve)
    META_API_OBJECT_CONVERTIBLE(META_NS::ICubicBezier)
    META_API_CACHE_INTERFACE(META_NS::ICubicBezier, CubicBezier)
public:
    META_API_INTERFACE_PROPERTY_CACHED(CubicBezier, ControlPoint1, BASE_NS::Math::Vec2)
    META_API_INTERFACE_PROPERTY_CACHED(CubicBezier, ControlPoint2, BASE_NS::Math::Vec2)
};

} // namespace Easing
} // namespace Curves

META_END_NAMESPACE()

#endif // META_API_CUBIC_BEZIER_EASING_CURVE_H
