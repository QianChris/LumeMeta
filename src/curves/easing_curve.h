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
#ifndef META_SRC_EASING_CURVE_H
#define META_SRC_EASING_CURVE_H

#include <meta/base/namespace.h>
#include <meta/ext/object_factory.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/curves/intf_easing_curve.h>
#include <meta/interface/intf_object_registry.h>

#include "../base_object.h"

META_BEGIN_NAMESPACE()

namespace Curves {
namespace Easing {

/**
 * @brief A single data-driven easing curve. Every built-in easing curve (Linear, InQuad, OutBack, …)
 *        is this one class configured with the transform function for that curve, registered under
 *        its own ClassId via EasingCurveFactory. This replaces ~33 near-identical wrapper classes
 *        (each of which cost a vtable + object factory + metadata) with one class + a function table.
 */
class EasingCurve final : public IntroduceInterfaces<BaseObject, IEasingCurve> {
    META_OBJECT_NO_CLASSINFO(EasingCurve, IntroduceInterfaces)
public:
    using TransformFunc = float (*)(float);

    // Bound to a specific built-in curve by EasingCurveFactory at construction.
    EasingCurve(const ObjectId& id, BASE_NS::string_view name, TransformFunc fn)
        : classId_(id), className_(name), func_(fn)
    {}

    float Transform(float t) const override
    {
        return func_ ? func_(t) : t;
    }

    // Per-instance class identity (instead of META_OBJECT's single hard-coded ClassId).
    ObjectId GetClassId() const override
    {
        return classId_;
    }
    BASE_NS::string_view GetClassName() const override
    {
        return className_;
    }
    BASE_NS::vector<BASE_NS::Uid> GetInterfaces() const override
    {
        return GetInterfacesVector();
    }

private:
    ObjectId classId_;
    BASE_NS::string_view className_;
    TransformFunc func_{};
};

/**
 * @brief Data-driven factory: one factory instance per built-in easing curve, carrying that curve's
 *        ClassInfo + transform function. Shares EasingCurve's metadata/interfaces.
 */
class EasingCurveFactory final : public ObjectFactory {
public:
    EasingCurveFactory(const ClassInfo& info, EasingCurve::TransformFunc fn) : ObjectFactory(info), func_(fn)
    {}
    IObject::Ptr CreateInstance() const override
    {
        return IObject::Ptr{new EasingCurve(GetClassInfo().Id(), GetClassInfo().Name(), func_)};
    }
    const StaticObjectMetadata* GetClassStaticMetadata() const override
    {
        return EasingCurve::StaticMetadata();
    }
    const BASE_NS::vector<BASE_NS::Uid>& GetClassInterfaces() const override
    {
        static auto interfaces = EasingCurve::GetInterfacesVector();
        return interfaces;
    }
    ClassConstructionType ConstructionType() const override
    {
        return ClassConstructionType::BUILD;
    }

private:
    EasingCurve::TransformFunc func_;
};

void RegisterEasingCurves(IObjectRegistry& registry);
void UnregisterEasingCurves(IObjectRegistry& registry);

}  // namespace Easing
}  // namespace Curves

META_END_NAMESPACE()

#endif
