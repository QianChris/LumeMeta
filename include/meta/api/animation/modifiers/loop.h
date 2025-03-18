/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Loop animation modifier user API
 * Author: Sami Enne
 * Create: 2023-02-09
 */

#ifndef META_API_ANIMATION_MODIFIERS_LOOP_H
#define META_API_ANIMATION_MODIFIERS_LOOP_H

#include <meta/api/internal/animation_modifier_api.h>
#include <meta/base/namespace.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/modifiers/intf_loop.h>

META_BEGIN_NAMESPACE()

namespace AnimationModifiers {

/**
 * @brief Modifier for animations to create looping
 * Example:
 *     auto loop = AnimationModifiers::Loop().LoopCount(2);
 *     animation.Attach(loop);
 *   will run the original animation twice.
 * Default value for looping is 1 (i.e. not altering behaviour).
 */
class Loop final : public Internal::AnimationModifierInterfaceAPI<Loop, META_NS::ClassId::LoopAnimationModifier> {
    META_API(Loop)
    META_API_OBJECT_CONVERTIBLE(META_NS::AnimationModifiers::ILoop)
    META_API_CACHE_INTERFACE(META_NS::AnimationModifiers::ILoop, Loop)
public:
    /**
     * @brief Set how many times the animation should loop, negative value means keep looping indefinitely.
     */
    META_API_INTERFACE_PROPERTY_CACHED(Loop, LoopCount, int32_t)
    /**
     * @brief Make the animation to keep looping until explicitly stopped.
     */
    Loop& LoopIndefinitely()
    {
        META_NS::SetValue(META_API_CALL_INTERFACE(Loop, LoopCount()), -1);
        return *this;
    }
};

} // namespace AnimationModifiers

META_END_NAMESPACE()

#endif // META_API_ANIMATION_MODIFIERS_LOOP_H
