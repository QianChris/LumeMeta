/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2025. All rights reserved.
 * Description: Definition of animation interface wrapper objects.
 * Author: Lauri Jääskelä
 * Create: 2021-09-20
 */

#ifndef META_API_ANIMATION_H
#define META_API_ANIMATION_H

#include <meta/api/object.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/animation/intf_animation_modifier.h>
#include <meta/interface/animation/modifiers/intf_loop.h>
#include <meta/interface/animation/modifiers/intf_speed.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Wrapper class for for objects which implement IAnimation.
 */
class Animation : public Object {
public:
    META_INTERFACE_OBJECT(Animation, Object, IAnimation)

    /// @see IAnimation::Enabled
    META_INTERFACE_OBJECT_PROPERTY(bool, Enabled)
    /// @see IAnimation::Valid
    META_INTERFACE_OBJECT_READONLY_PROPERTY(bool, Valid)
    /// @see IAnimation::TotalDuration
    META_INTERFACE_OBJECT_READONLY_PROPERTY(TimeSpan, TotalDuration)
    /// @see IAnimation::Running
    META_INTERFACE_OBJECT_READONLY_PROPERTY(bool, Running)
    /// @see IAnimation::Progress
    META_INTERFACE_OBJECT_READONLY_PROPERTY(float, Progress)
    /// @see IAnimation::Curve
    META_INTERFACE_OBJECT_PROPERTY(ICurve1D::Ptr, Curve)
    /// @see IAnimation::Controller
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::weak_ptr<IAnimationController>, Controller)
    /**
     * @see IStartableAnimation::Pause
     * @note Does nothing if the underlying animation does not implement IStartableAnimation
     */
    void Pause()
    {
        CallPtr<IStartableAnimation>([](auto& p) { p.Pause(); });
    }
    /**
     * @see IStartableAnimation::Restart
     * @note Does nothing if the underlying animation does not implement IStartableAnimation
     */
    void Restart()
    {
        CallPtr<IStartableAnimation>([](auto& p) { p.Restart(); });
    }
    /**
     * @see IStartableAnimation::Seek
     * @note Does nothing if the underlying animation does not implement IStartableAnimation
     */
    void Seek(float position)
    {
        CallPtr<IStartableAnimation>([position](auto& p) { p.Seek(position); });
    }
    /**
     * @see IStartableAnimation::Start
     * @note Does nothing if the underlying animation does not implement IStartableAnimation
     */
    void Start()
    {
        CallPtr<IStartableAnimation>([](auto& p) { p.Start(); });
    }
    /**
     * @see IStartableAnimation::Stop
     * @note Does nothing if the underlying animation does not implement IStartableAnimation
     */
    void Stop()
    {
        CallPtr<IStartableAnimation>([](auto& p) { p.Stop(); });
    }
    /**
     * @see IStartableAnimation::Finish
     * @note Does nothing if the underlying animation does not implement IStartableAnimation
     */
    void Finish()
    {
        CallPtr<IStartableAnimation>([](auto& p) { p.Finish(); });
    }
    /// @see IAnimation::Step
    void Step(const IClock::ConstPtr& clock)
    {
        META_INTERFACE_OBJECT_CALL_PTR(Step(clock));
    }
    /// @see IAnimation::OnFinished
    auto OnFinished()
    {
        return META_INTERFACE_OBJECT_CALL_PTR(OnFinished());
    }
    /// @see IAnimation::OnStarted
    auto OnStarted()
    {
        return META_INTERFACE_OBJECT_CALL_PTR(OnStarted());
    }
    /**
     * @brief Add a modifier to the animation
     * @param modifier The modifier to add
     * @return True if the modifier was successfully applied, false otherwise.
     */
    bool Modifier(const IAnimationModifier::Ptr& modifier)
    {
        return CallPtr<IAttach>([&](auto& p) { return p.Attach(modifier); });
    }
};

/**
 * @brief Wrapper class for ITimedAnimation.
 */
class TimedAnimation : public Animation {
public:
    META_INTERFACE_OBJECT(TimedAnimation, Animation, ITimedAnimation)
    META_INTERFACE_OBJECT_PROPERTY(TimeSpan, Duration)
};

/**
 * @brief Wrapper class for IPropertyAnimation.
 */
class PropertyAnimation : public TimedAnimation {
public:
    META_INTERFACE_OBJECT(PropertyAnimation, TimedAnimation, IPropertyAnimation)
    META_INTERFACE_OBJECT_INSTANTIATE(PropertyAnimation, ClassId::PropertyAnimation)

    /// @see IPropertyAnimation::Property
    META_INTERFACE_OBJECT_PROPERTY(IProperty::WeakPtr, Property)
};

/**
 * @brief Typed wrapper class for objects which implement IKeyframeAnimation.
 */
template<typename Type>
class KeyframeAnimation : public PropertyAnimation {
public:
    META_INTERFACE_OBJECT(KeyframeAnimation<Type>, PropertyAnimation, IKeyframeAnimation)
    META_INTERFACE_OBJECT_INSTANTIATE(KeyframeAnimation<Type>, ClassId::KeyframeAnimation)
    /// @see IKeyframeAnimation::From
    auto From()
    {
        return META_INTERFACE_OBJECT_CALL_PTR(From());
    }
    /// @see IKeyframeAnimation::To
    auto To()
    {
        return META_INTERFACE_OBJECT_CALL_PTR(To());
    }
    auto& From(const Type& value)
    {
        SetAnyPtrProperty(From(), value);
        return *this;
    }
    auto& To(const Type& value)
    {
        SetAnyPtrProperty(To(), value);
        return *this;
    }

private:
    template<typename PropertyType>
    void SetAnyPtrProperty(const PropertyType& p, const Type& value)
    {
        if (p) {
            if (auto pv = p->GetValue()) { // IAny::Ptr
                pv->SetValue(value);
            } else {
                p->SetValue(ConstructAny<Type>(value));
            }
        }
    }
};

/**
 * @brief Typed wrapper class for objects which implement ITrackAnimation.
 */
template<typename T>
class TrackAnimation : public PropertyAnimation {
public:
    META_INTERFACE_OBJECT(TrackAnimation<T>, PropertyAnimation, ITrackAnimation)
    META_INTERFACE_OBJECT_INSTANTIATE(TrackAnimation<T>, ClassId::TrackAnimation)

    /// @see ITrackAnimation::Timestamps
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::array_view<const float>, Timestamps)
    /// Typed access to ITrackAnimation::Keyframes property.
    ArrayProperty<T> Keyframes() const
    {
        auto kf = META_INTERFACE_OBJECT_CALL_PTR(Keyframes());
        if (auto internal = interface_cast<META_NS::IPropertyInternalAny>(kf)) {
            if (!internal->GetInternalAny()) {
                internal->SetInternalAny(ConstructArrayAny<T>());
            }
        }
        return ArrayProperty<T>(kf);
    }
    /// Set ITrackAnimation::Keyframes property from a typed array.
    auto& Keyframes(const BASE_NS::array_view<const T> keyframes)
    {
        if (auto kf = Keyframes()) {
            kf->SetValue(keyframes);
        }
        return *this;
    }
    /// @see ITrackAnimation::KeyframeCurves
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::array_view<const ICurve1D::Ptr>, KeyframeCurves)
    /// @see ITrackAnimation::KeyframeHandlers
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::array_view<const IFunction::Ptr>, KeyframeHandlers)
    /// @see ITrackAnimation::CurrentKeyframeIndex
    META_INTERFACE_OBJECT_READONLY_PROPERTY(uint32_t, CurrentKeyframeIndex)
    /// @see ITrackAnimation::AddKeyframe
    auto AddKeyframe(float timestamp, const IAny::ConstPtr& value)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(AddKeyframe(timestamp, value));
    }
    /// @see ITrackAnimation::RemoveKeyframe
    auto RemoveKeyframe(size_t index)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(RemoveKeyframe(index));
    }
    /// @see ITrackAnimation::RemoveAllKeyframes
    void RemoveAllKeyframes()
    {
        META_INTERFACE_OBJECT_CALL_PTR(RemoveAllKeyframes());
    }
};

/**
 * @brief Wrapper class for animation containers.
 */
class StaggeredAnimation : public Animation {
public:
    META_INTERFACE_OBJECT(StaggeredAnimation, Animation, IStaggeredAnimation)
    auto& Add(const IAnimation::Ptr& animation)
    {
        META_INTERFACE_OBJECT_CALL_PTR(AddAnimation(animation));
        return *this;
    }
    auto& Remove(const IAnimation::Ptr& animation)
    {
        META_INTERFACE_OBJECT_CALL_PTR(RemoveAnimation(animation));
        return *this;
    }
    BASE_NS::vector<IAnimation::Ptr> GetAnimations() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetAnimations());
    }
};

/**
 * @brief Wrapper class for objects which implement ISequentialAnimation.
 */
class SequentialAnimation : public StaggeredAnimation {
public:
    META_INTERFACE_OBJECT(SequentialAnimation, StaggeredAnimation, ISequentialAnimation)
    META_INTERFACE_OBJECT_INSTANTIATE(SequentialAnimation, ClassId::SequentialAnimation)
};

/**
 * @brief Wrapper class for objects which implement IParallelAnimation.
 */
class ParallelAnimation : public StaggeredAnimation {
public:
    META_INTERFACE_OBJECT(ParallelAnimation, StaggeredAnimation, IParallelAnimation)
    META_INTERFACE_OBJECT_INSTANTIATE(ParallelAnimation, ClassId::ParallelAnimation)
};

/**
 * @brief Wrapper class for objects which implement IAnimationModifier.
 */
class AnimationModifier : public InterfaceObject<IAnimationModifier> {
public:
    META_INTERFACE_OBJECT(AnimationModifier, InterfaceObject<IAnimationModifier>, IAnimationModifier)
};

namespace AnimationModifiers {

/**
 * @brief Wrapper class for objects which implement ILoop.
 */
class Loop : public AnimationModifier {
public:
    META_INTERFACE_OBJECT(Loop, AnimationModifier, ILoop)
    META_INTERFACE_OBJECT_INSTANTIATE(Loop, ClassId::LoopAnimationModifier)
    /// @see ILoop::LoopCount
    META_INTERFACE_OBJECT_PROPERTY(int32_t, LoopCount)
    /// Set the modifier to loop indefinitely.
    auto& LoopIndefinitely()
    {
        META_NS::SetValue(LoopCount(), -1);
        return *this;
    }
};

/**
 * @brief Wrapper class for objects which implement ISpeed.
 */
class Speed : public AnimationModifier {
public:
    META_INTERFACE_OBJECT(Speed, AnimationModifier, ISpeed)
    META_INTERFACE_OBJECT_INSTANTIATE(Speed, ClassId::SpeedAnimationModifier)
    /// @see ISpeed::SpeedFactor
    META_INTERFACE_OBJECT_PROPERTY(float, SpeedFactor)
};

/**
 * @brief Wrapper class for reverse animations.
 */
class Reverse : public InterfaceObject<IAnimationModifier> {
public:
    META_INTERFACE_OBJECT(Reverse, InterfaceObject<IAnimationModifier>, IAnimationModifier)
    META_INTERFACE_OBJECT_INSTANTIATE(Reverse, ClassId::ReverseAnimationModifier)
};
} // namespace AnimationModifiers

/// Returns a default object which implements IPropertyAnimation
template<>
inline auto CreateInstance<IPropertyAnimation>()
{
    return PropertyAnimation(CreateNew);
}

/**
 * @brief Returns a default typed animation object for given interface and type.
 * @note Supported interfaces are IKeyframeAnimation and ITrackAnimation
 */
template<typename Interface, typename Type>
inline auto CreateInstance()
{
    constexpr auto isKeyframeAnimation = BASE_NS::is_same_v<Interface, IKeyframeAnimation>;
    constexpr auto isTrackAnimation = BASE_NS::is_same_v<Interface, ITrackAnimation>;
    static_assert(isKeyframeAnimation || isTrackAnimation, "Invalid interface type for typed animation instantiation.");
    if constexpr (isKeyframeAnimation) {
        return KeyframeAnimation<Type>(CreateNew);
    }
    if constexpr (isTrackAnimation) {
        return TrackAnimation<Type>(CreateNew);
    }
}

/// Returns a default object which implements IKeyframeAnimation
template<>
inline auto CreateInstance<IParallelAnimation>()
{
    return ParallelAnimation(CreateNew);
}
/// Returns a default object which implements IKeyframeAnimation
template<>
inline auto CreateInstance<ISequentialAnimation>()
{
    return SequentialAnimation(CreateNew);
}

META_END_NAMESPACE()

#endif // META_API_ANIMATION_H
