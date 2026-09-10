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

#ifndef META_SRC_ALIAS_ALIAS_GROUP_H
#define META_SRC_ALIAS_ALIAS_GROUP_H

#include <meta/interface/alias/intf_alias_group.h>
#include <meta/interface/alias/intf_alias_registry.h>

#include <meta/ext/object.h>

META_BEGIN_NAMESPACE()

class AliasRegistry;

/**
 * @brief A handle on one layer of an alias registry.
 *
 * Holds no values of its own: a group is nothing but a path, and every contribution made through it
 * lives in the registry, keyed by that path. Two handles on the same path are therefore
 * interchangeable, and a handle kept across a RemoveGroup simply addresses an empty layer.
 */
class AliasGroup : public META_NS::IntroduceInterfaces<IAliasGroup> {
public:
    AliasGroup(BASE_NS::string path, const IAliasRegistry::Ptr& owner, AliasRegistry& registry, AliasScopePtr scope);

    // IAliasGroup
    BASE_NS::string GetPath() const override;
    bool Set(BASE_NS::array_view<const AliasEntry> values) override;
    bool Define(BASE_NS::string_view name, const META_NS::IAny& value) override;
    bool Define(BASE_NS::string_view name, const META_NS::IAny& value, const TransitionSpec& transition) override;
    bool Mask(BASE_NS::string_view name) override;
    bool Undefine(BASE_NS::string_view name) override;
    BASE_NS::vector<BASE_NS::string> GetNames() const override;
    void Clear() override;
    void SetTransition(const TransitionSpec& transition) override;
    TransitionSpec GetTransition() const override;

private:
    const BASE_NS::string path_;
    // The registry owns the layer, not the handle, and does not keep the handle alive; a handle may
    // outlive it. Locking `owner_` is what makes `registry_` safe to dereference — the raw pointer is
    // valid exactly for as long as that lock is held.
    const IAliasRegistry::WeakPtr owner_;
    AliasRegistry* const registry_;
    // The scope this handle addresses, fixed when it was handed out. A group is a path within one scope,
    // so the same path in two scopes is two layers, exactly as the same alias name is two cells.
    const AliasScopePtr scope_;
};

META_END_NAMESPACE()

#endif
