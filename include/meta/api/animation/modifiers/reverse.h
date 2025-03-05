/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Reverse animation modifier API
 * Author: Lauri Jaaskela
 * Create: 2023-07-25
 */

#ifndef META_API_ANIMATION_MODIFIERS_REVERSE_H
#define META_API_ANIMATION_MODIFIERS_REVERSE_H

#include <meta/api/internal/animation_modifier_api.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/modifiers/intf_loop.h>

META_BEGIN_NAMESPACE()

namespace AnimationModifiers {

/**
 * @brief Modifier for animations to run backwards
 */
class Reverse final
    : public Internal::AnimationModifierInterfaceAPI<Reverse, META_NS::ClassId::ReverseAnimationModifier> {
    META_API(Reverse)
public:
};
} // namespace AnimationModifiers

META_END_NAMESPACE()

#endif // META_API_ANIMATION_MODIFIERS_REVERSE_H
