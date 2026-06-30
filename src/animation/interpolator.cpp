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

#include "interpolator.h"

#include <base/math/quaternion_util.h>

#include <meta/ext/object_factory.h>

META_BEGIN_NAMESPACE()

namespace {

using InterpolateFunc = AnyReturnValue (*)(IAny&, const IAny&, const IAny&, float);

// Generic lerp for types with arithmetic operators (scalars, float vectors, UVec2/IVec2).
template<class Type>
AnyReturnValue InterpolateLerp(IAny& output, const IAny& from, const IAny& to, float t)
{
    if (IsGetCompatibleWith<Type>(output) && IsGetCompatibleWith<Type>(from) && IsGetCompatibleWith<Type>(to)) {
        Type value0 = GetValue<Type>(from);
        Type value1 = GetValue<Type>(to);
        auto result = value0 + (value1 - value0) * t;
        if constexpr (BASE_NS::is_integral_v<Type>) {
            return output.SetValue<Type>(static_cast<Type>(BASE_NS::Math::round(result)));
        } else {
            return output.SetValue<Type>(result);
        }
    }
    return AnyReturn::INCOMPATIBLE_TYPE;
}

AnyReturnValue InterpolateQuat(IAny& output, const IAny& from, const IAny& to, float t)
{
    using Type = BASE_NS::Math::Quat;
    if (IsSetCompatibleWith<Type>(output) && IsGetCompatibleWith<Type>(from) && IsGetCompatibleWith<Type>(to)) {
        Type value0 = GetValue<Type>(from);
        Type value1 = GetValue<Type>(to);
        return output.SetValue<Type>(BASE_NS::Math::Slerp(value0, value1, t));
    }
    return AnyReturn::INCOMPATIBLE_TYPE;
}

template<typename T>
T InterpolateComponent(T a, T b, float t)
{
    return static_cast<T>(BASE_NS::Math::round(a + (b - a) * t));
}

// No math operators for integer vec3/vec4; interpolate per component.
template<class Type>
AnyReturnValue InterpolateIntVec3(IAny& output, const IAny& from, const IAny& to, float t)
{
    if (IsSetCompatibleWith<Type>(output) && IsGetCompatibleWith<Type>(from) && IsGetCompatibleWith<Type>(to)) {
        Type value0 = GetValue<Type>(from);
        Type value1 = GetValue<Type>(to);
        Type value;
        value.x = InterpolateComponent(value0.x, value1.x, t);
        value.y = InterpolateComponent(value0.y, value1.y, t);
        value.z = InterpolateComponent(value0.z, value1.z, t);
        return output.SetValue<Type>(value);
    }
    return AnyReturn::INCOMPATIBLE_TYPE;
}

template<class Type>
AnyReturnValue InterpolateIntVec4(IAny& output, const IAny& from, const IAny& to, float t)
{
    if (IsSetCompatibleWith<Type>(output) && IsGetCompatibleWith<Type>(from) && IsGetCompatibleWith<Type>(to)) {
        Type value0 = GetValue<Type>(from);
        Type value1 = GetValue<Type>(to);
        Type value;
        value.x = InterpolateComponent(value0.x, value1.x, t);
        value.y = InterpolateComponent(value0.y, value1.y, t);
        value.z = InterpolateComponent(value0.z, value1.z, t);
        value.w = InterpolateComponent(value0.w, value1.w, t);
        return output.SetValue<Type>(value);
    }
    return AnyReturn::INCOMPATIBLE_TYPE;
}

// The default interpolator doesn't know how to interpolate, so it just snaps to from or to.
AnyReturnValue InterpolateDefault(IAny& output, const IAny& from, const IAny& to, float t)
{
    return t > .5f ? output.CopyFrom(to) : output.CopyFrom(from);
}

/**
 * @brief A single data-driven interpolator. Each built-in interpolator (Float, Vec3, Quat, …) is
 *        this one class configured with the interpolation function + property type for that type,
 *        registered under its own ClassId via InterpolatorFactory. Replaces ~20 wrapper classes
 *        (each costing a vtable + object factory + metadata) with one class + a function table.
 *        (The public Interpolator<T> template stays for user-defined interpolators.)
 */
class BuiltinInterpolator final : public IntroduceInterfaces<BaseObject, IInterpolator> {
    META_OBJECT_NO_CLASSINFO(BuiltinInterpolator, IntroduceInterfaces)
public:
    BuiltinInterpolator(const ObjectId& id, BASE_NS::string_view name, TypeId typeUid, bool anyType, InterpolateFunc fn)
        : classId_(id), className_(name), typeUid_(typeUid), anyType_(anyType), func_(fn)
    {}
    AnyReturnValue Interpolate(IAny& output, const IAny& from, const IAny& to, float t) const override
    {
        return func_(output, from, to, t);
    }
    bool IsCompatibleWith(TypeId id) const noexcept override
    {
        return anyType_ || id == typeUid_;
    }
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
    TypeId typeUid_;
    bool anyType_{false};
    InterpolateFunc func_{};
};

class InterpolatorFactory final : public ObjectFactory {
public:
    InterpolatorFactory(const ClassInfo& info, TypeId typeUid, bool anyType, InterpolateFunc fn)
        : ObjectFactory(info), typeUid_(typeUid), anyType_(anyType), func_(fn)
    {}
    IObject::Ptr CreateInstance() const override
    {
        return IObject::Ptr{
            new BuiltinInterpolator(GetClassInfo().Id(), GetClassInfo().Name(), typeUid_, anyType_, func_)};
    }
    const StaticObjectMetadata* GetClassStaticMetadata() const override
    {
        return BuiltinInterpolator::StaticMetadata();
    }
    const BASE_NS::vector<BASE_NS::Uid>& GetClassInterfaces() const override
    {
        static auto interfaces = BuiltinInterpolator::GetInterfacesVector();
        return interfaces;
    }
    ClassConstructionType ConstructionType() const override
    {
        return ClassConstructionType::BUILD;
    }
    TypeId PropertyTypeUid() const
    {
        return typeUid_;
    }
    bool IsTypeMapped() const
    {
        return !anyType_;
    }

private:
    TypeId typeUid_;
    bool anyType_;
    InterpolateFunc func_;
};

#define LERP_INTERPOLATOR(name, type)                                    \
    {                                                                    \
        ClassId::name, UidFromType<type>(), false, InterpolateLerp<type> \
    }
static InterpolatorFactory g_interpolatorFactories[] = {
    LERP_INTERPOLATOR(FloatInterpolator, float),
    LERP_INTERPOLATOR(DoubleInterpolator, double),
    LERP_INTERPOLATOR(Vec2Interpolator, BASE_NS::Math::Vec2),
    LERP_INTERPOLATOR(Vec3Interpolator, BASE_NS::Math::Vec3),
    LERP_INTERPOLATOR(Vec4Interpolator, BASE_NS::Math::Vec4),
    LERP_INTERPOLATOR(UVec2Interpolator, BASE_NS::Math::UVec2),
    LERP_INTERPOLATOR(IVec2Interpolator, BASE_NS::Math::IVec2),
    LERP_INTERPOLATOR(UInt8Interpolator, uint8_t),
    LERP_INTERPOLATOR(UInt16Interpolator, uint16_t),
    LERP_INTERPOLATOR(UInt32Interpolator, uint32_t),
    LERP_INTERPOLATOR(UInt64Interpolator, uint64_t),
    LERP_INTERPOLATOR(Int8Interpolator, int8_t),
    LERP_INTERPOLATOR(Int16Interpolator, int16_t),
    LERP_INTERPOLATOR(Int32Interpolator, int32_t),
    LERP_INTERPOLATOR(Int64Interpolator, int64_t),
    {ClassId::QuatInterpolator, UidFromType<BASE_NS::Math::Quat>(), false, InterpolateQuat},
    {ClassId::UVec3Interpolator, UidFromType<BASE_NS::Math::UVec3>(), false, InterpolateIntVec3<BASE_NS::Math::UVec3>},
    {ClassId::UVec4Interpolator, UidFromType<BASE_NS::Math::UVec4>(), false, InterpolateIntVec4<BASE_NS::Math::UVec4>},
    {ClassId::IVec3Interpolator, UidFromType<BASE_NS::Math::IVec3>(), false, InterpolateIntVec3<BASE_NS::Math::IVec3>},
    {ClassId::IVec4Interpolator, UidFromType<BASE_NS::Math::IVec4>(), false, InterpolateIntVec4<BASE_NS::Math::IVec4>},
};
#undef LERP_INTERPOLATOR

// Default interpolator: registered as a class only (not mapped to a property type), compatible with any.
static InterpolatorFactory g_defaultInterpolatorFactory{ClassId::DefaultInterpolator, {}, true, InterpolateDefault};

}  // namespace

void RegisterDefaultInterpolators(IObjectRegistry& registry)
{
    for (auto& f : g_interpolatorFactories) {
        registry.RegisterObjectType(IObjectFactory::Ptr{&f});
        registry.RegisterInterpolator(f.PropertyTypeUid(), f.GetClassInfo().Id().ToUid());
    }
    // Default interpolator: only register the class, it is not mapped to a property type.
    registry.RegisterObjectType(IObjectFactory::Ptr{&g_defaultInterpolatorFactory});
}
void UnRegisterDefaultInterpolators(IObjectRegistry& registry)
{
    for (auto& f : g_interpolatorFactories) {
        registry.UnregisterInterpolator(f.PropertyTypeUid());
        registry.UnregisterObjectType(IObjectFactory::Ptr{&f});
    }
    registry.UnregisterObjectType(IObjectFactory::Ptr{&g_defaultInterpolatorFactory});
}

META_END_NAMESPACE()
