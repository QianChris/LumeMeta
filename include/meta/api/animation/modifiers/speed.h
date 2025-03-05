/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Speed animation modifier API
 * Author: Lauri Jaaskela
 * Create: 2023-07-25
 */

#ifndef META_API_ANIMATION_MODIFIERS_SPEED_H
#define META_API_ANIMATION_MODIFIERS_SPEED_H

#include <meta/api/internal/animation_modifier_api.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/modifiers/intf_speed.h>

META_BEGIN_NAMESPACE()

namespace AnimationModifiers {

/**
 * @brief Modifier for animations to set a playback speed factor for the animation.
 * Example:
 *     animation.Attach(AnimationModifiers::Speed().Speed(0.5f));
 *   will run the original animation at half speed.
 * Default value for speed is 1 (i.e. not altering behaviour).
 */
class Speed final : public Internal::AnimationModifierInterfaceAPI<Speed, META_NS::ClassId::SpeedAnimationModifier> {
    META_API(Speed)
    META_API_OBJECT_CONVERTIBLE(META_NS::AnimationModifiers::ISpeed)
    META_API_CACHE_INTERFACE(META_NS::AnimationModifiers::ISpeed, Speed)
public:
    /**
     * @brief Set the speed factor.
     */
    META_API_INTERFACE_PROPERTY_CACHED(Speed, SpeedFactor, float)
};

} // namespace AnimationModifiers

META_END_NAMESPACE()

#endif // META_API_ANIMATION_MODIFIERS_SPEED_H
