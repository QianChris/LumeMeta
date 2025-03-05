/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Animation modifier forwarder api
 * Author: Sami Enne
 * Create: 2023-02-09
 */

#ifndef META_API_ANIMATION_MODIFIER_API_H
#define META_API_ANIMATION_MODIFIER_API_H

#include <meta/api/internal/object_api.h>
#include <meta/base/namespace.h>
#include <meta/interface/animation/intf_animation_modifier.h>

META_BEGIN_NAMESPACE()

namespace Internal {

template<class FinalClass, const META_NS::ClassInfo& Class>
class AnimationModifierInterfaceAPI : public ObjectInterfaceAPI<FinalClass, Class> {
    META_INTERFACE_API(AnimationModifierInterfaceAPI)
    META_API_OBJECT_CONVERTIBLE(META_NS::IAnimationModifier)
    META_API_OBJECT_CONVERTIBLE(META_NS::IAttachment)
public:
};

} // namespace Internal

META_END_NAMESPACE()

#endif // META_API_ANIMATION_MODIFIER_API_H
