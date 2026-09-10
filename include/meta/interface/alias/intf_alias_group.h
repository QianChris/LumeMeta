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

#ifndef META_INTERFACE_ALIAS_IALIAS_GROUP_H
#define META_INTERFACE_ALIAS_IALIAS_GROUP_H

#include <meta/base/namespace.h>
#include <meta/interface/animation/transition.h>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>

#include <meta/base/interface_macros.h>
#include <meta/base/time_span.h>
#include <meta/base/types.h>
#include <meta/interface/curves/intf_curve_1d.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_any.h>

META_BEGIN_NAMESPACE()

/**
 * @brief One alias name and what a group contributes for it. A null value is a mask: the group states
 *        that the alias has no value here, which blocks the groups below it from supplying one.
 */
struct AliasEntry {
    BASE_NS::string name;
    META_NS::IAny::Ptr value;
    /**
     * How this contribution arrives, and how it leaves again when it is withdrawn. Carried on the
     * contribution rather than looked up when something changes, which is what makes withdrawing a value
     * unambiguous: a layer leaves the way it came, whatever has been applied over it since.
     */
    TransitionSpec transition;
};

/**
 * @brief A removable layer of alias values in an IAliasRegistry.
 *
 * A group is addressed by a slash-separated path ("theme", "theme/accent") and depth is priority: a
 * nested group overrides the group it is nested in, so layering is stated by the path rather than by
 * the order in which value sets happen to be loaded. Groups at the same depth should not define the
 * same alias; where they do, the last contribution applied wins.
 *
 * Below every group sits the alias's default (IAliasRegistry::SetDefault); above every group sit
 * ungrouped definitions (IAliasRegistry::Define), which no group can override.
 *
 * A group exists exactly as long as it contributes something. Emptying it (@ref Clear, or a @ref Set
 * with no entries) removes it, and every alias it decided falls back to the next layer down.
 */
class IAliasGroup : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IAliasGroup, "6e6094e9-5ded-46d5-be47-265f3b95eb50")
public:
    /**
     * @brief The path this group occupies.
     */
    virtual BASE_NS::string GetPath() const = 0;

    /**
     * @brief Replace everything this group contributes with @p values. Aliases the group used to decide
     *        and @p values omits are released, so applying one value set over another leaves nothing of
     *        the first behind. Returns true only if every entry was applied (an entry fails on a type
     *        conflict with an established alias).
     *
     *        Each entry is taken as fully described, its transition included: unlike the single-value
     *        calls, nothing here inherits @ref GetTransition, because an entry that means to inherit and
     *        one that means to be instant would otherwise look the same. A caller assembling entries by
     *        hand fills them from @ref GetTransition where it wants that.
     */
    virtual bool Set(BASE_NS::array_view<const AliasEntry> values) = 0;

    /**
     * @brief Contribute @p value for the alias @p name, replacing whatever this group contributed for it
     *        before. Returns false on a type conflict with an already-established alias of that name.
     *        States no transition of its own, so the contribution inherits @ref GetTransition.
     */
    virtual bool Define(BASE_NS::string_view name, const META_NS::IAny& value) = 0;

    /**
     * @brief As above, with the transition this one value arrives and leaves by stated outright. A
     *        default-constructed @p transition means instant, and is how one value opts out of a
     *        transition the group would otherwise give it.
     */
    virtual bool Define(BASE_NS::string_view name, const META_NS::IAny& value, const TransitionSpec& transition) = 0;

    /**
     * @brief Contribute "no value" for the alias @p name: everything below this group is blocked from
     *        supplying one — in a scope, the global alias included — and the alias falls back to its
     *        default. Valid before the alias exists, so a mask does not depend on the masked group
     *        having been applied first.
     *
     *        A group that means to leave the layers below it alone contributes nothing for the name
     *        instead (@ref Undefine, or a @ref Set that omits it).
     */
    virtual bool Mask(BASE_NS::string_view name) = 0;

    /**
     * @brief Stop contributing anything for the alias @p name; it falls back to the next layer down.
     *        Returns false if this group contributed nothing for it.
     */
    virtual bool Undefine(BASE_NS::string_view name) = 0;

    /**
     * @brief The alias names this group contributes to, masks included.
     */
    virtual BASE_NS::vector<BASE_NS::string> GetNames() const = 0;

    /**
     * @brief Release everything this group contributes, removing the group.
     */
    virtual void Clear() = 0;

    /**
     * @brief How values applied to this group arrive, and how they leave again — the transition used for
     *        a contribution that states none of its own. It covers the groups nested under this one too,
     *        so a spec on "theme" governs "theme/accent" unless that group sets one, which is what lets a
     *        set of interchangeable themes state once how switching between them behaves.
     *
     *        This is a setting, not a contribution: it survives @ref Clear and IAliasRegistry::RemoveGroup
     *        (how a layer behaves is not something unloading its content should forget), it does not make
     *        the group exist, and it may be set on a group nothing has been applied to yet.
     */
    virtual void SetTransition(const TransitionSpec& transition) = 0;

    /**
     * @brief The transition a value applied to this group gets when it states none of its own —  this
     *        group's, or the nearest one up its path, or default-constructed when no group on the path
     *        sets one. This is what a value set resolves against, and what answers "how would applying
     *        something here behave".
     */
    virtual TransitionSpec GetTransition() const = 0;
};

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IAliasGroup)

#endif
