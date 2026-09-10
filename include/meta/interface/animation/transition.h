/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef META_INTERFACE_ANIMATION_TRANSITION_H
#define META_INTERFACE_ANIMATION_TRANSITION_H

#include <meta/base/namespace.h>
#include <meta/base/time_span.h>
#include <meta/interface/curves/intf_curve_1d.h>

META_BEGIN_NAMESPACE()

/**
 * @brief How a value change plays out over time rather than taking effect at once.
 *
 * A transition describes a change, not the thing being changed: whoever causes the change decides how
 * long it takes, and carries the spec with what they did. Holding it that way is what lets a change be
 * undone the way it was made, without having to ask anything about where the value has since ended up.
 *
 * A zero duration, which is what a default-constructed spec holds, is an instant change. So is a change
 * of a type nothing can meaningfully interpolate, and so is the first value something ever receives,
 * since there is nothing to move from.
 *
 * In JSON this is @c types.schema.json#/$defs/transition, shared by every document that can state one.
 */
struct TransitionSpec {
    /** How long the change takes. Zero is instant. */
    TimeSpan duration;
    /** Eased over the duration; null is linear. */
    ICurve1D::Ptr curve;
};

META_END_NAMESPACE()

#endif
