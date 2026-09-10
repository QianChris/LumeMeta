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

#ifndef META_API_ALIAS_H
#define META_API_ALIAS_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>

#include <meta/base/namespace.h>
#include <meta/interface/alias/intf_alias_registry.h>

META_BEGIN_NAMESPACE()

/**
 * @brief An alias registry pinned to one scope.
 *
 * Every registry call takes a scope, because that is where the machinery needs it. This exists so that
 * application code does not repeat it: ask for the scope once and then say what you mean.
 *
 * A default-constructed scope, or one built from a null registry, is inert — every call returns as it
 * would for an alias that does not exist, so a caller need not check before each one.
 */
class AliasScope {
public:
    AliasScope() = default;
    explicit AliasScope(IAliasRegistry::Ptr registry, AliasScopePtr scope = {})
        : registry_(BASE_NS::move(registry)), scope_(BASE_NS::move(scope))
    {}

    /// True when there is a registry to talk to.
    explicit operator bool() const noexcept
    {
        return registry_ != nullptr;
    }

    /// @see IAliasRegistry::Bind
    IProperty::Ptr Bind(const IProperty::Ptr& target, BASE_NS::string_view name) const
    {
        return registry_ ? registry_->Bind(target, name, scope_) : nullptr;
    }
    /// @see IAliasRegistry::Unbind
    bool Unbind(const IProperty::Ptr& target) const
    {
        return registry_ && registry_->Unbind(target);
    }
    /// @see IAliasRegistry::Define
    bool Define(BASE_NS::string_view name, const IAny& value) const
    {
        return registry_ && registry_->Define(name, value, scope_);
    }
    /// @see IAliasRegistry::Undefine
    bool Undefine(BASE_NS::string_view name) const
    {
        return registry_ && registry_->Undefine(name, scope_);
    }
    /// @see IAliasRegistry::Remove
    bool Remove(BASE_NS::string_view name) const
    {
        return registry_ && registry_->Remove(name, scope_);
    }
    /// @see IAliasRegistry::SetDefault
    IProperty::Ptr SetDefault(BASE_NS::string_view name, const IAny::Ptr& defaultValue) const
    {
        return registry_ ? registry_->SetDefault(name, defaultValue, scope_) : nullptr;
    }
    /// @see IAliasRegistry::GetAlias
    IProperty::Ptr GetAlias(BASE_NS::string_view name) const
    {
        return registry_ ? registry_->GetAlias(name, scope_) : nullptr;
    }
    /// @see IAliasRegistry::GetAliases
    BASE_NS::vector<AliasInfo> GetAliases() const
    {
        return registry_ ? registry_->GetAliases(scope_) : BASE_NS::vector<AliasInfo>{};
    }
    /// @see IAliasRegistry::GetAliasType
    TypeId GetAliasType(BASE_NS::string_view name) const
    {
        return registry_ ? registry_->GetAliasType(name, scope_) : TypeId{};
    }
    /// @see IAliasRegistry::SetAliasTypeHint
    bool SetAliasTypeHint(BASE_NS::string_view name, const TypeId& type, BASE_NS::string_view group = {}) const
    {
        return registry_ && registry_->SetAliasTypeHint(name, type, group, scope_);
    }
    /// @see IAliasRegistry::RemoveAliasTypeHint
    bool RemoveAliasTypeHint(BASE_NS::string_view name) const
    {
        return registry_ && registry_->RemoveAliasTypeHint(name, scope_);
    }
    /// @see IAliasRegistry::GetConsumers
    BASE_NS::vector<IProperty::Ptr> GetConsumers(BASE_NS::string_view name) const
    {
        return registry_ ? registry_->GetConsumers(name, scope_) : BASE_NS::vector<IProperty::Ptr>{};
    }
    /// @see IAliasRegistry::GetGroup
    IAliasGroup::Ptr GetGroup(BASE_NS::string_view path) const
    {
        return registry_ ? registry_->GetGroup(path, scope_) : nullptr;
    }
    /// @see IAliasRegistry::FindGroup
    IAliasGroup::Ptr FindGroup(BASE_NS::string_view path) const
    {
        return registry_ ? registry_->FindGroup(path, scope_) : nullptr;
    }
    /// @see IAliasRegistry::GetUngrouped
    IAliasGroup::Ptr GetUngrouped() const
    {
        return registry_ ? registry_->GetUngrouped(scope_) : nullptr;
    }
    /// @see IAliasRegistry::RemoveGroup
    bool RemoveGroup(BASE_NS::string_view path) const
    {
        return registry_ && registry_->RemoveGroup(path, scope_);
    }
    /// @see IAliasRegistry::GetContributingGroups
    BASE_NS::vector<BASE_NS::string> GetContributingGroups() const
    {
        return registry_ ? registry_->GetContributingGroups(scope_) : BASE_NS::vector<BASE_NS::string>{};
    }
    /// @see IAliasRegistry::GetSettingsGroups
    BASE_NS::vector<BASE_NS::string> GetSettingsGroups() const
    {
        return registry_ ? registry_->GetSettingsGroups(scope_) : BASE_NS::vector<BASE_NS::string>{};
    }

    /// The registry itself, for anything this does not wrap.
    const IAliasRegistry::Ptr& Registry() const noexcept
    {
        return registry_;
    }
    /// The scope every call above is made in; null is the global one.
    const AliasScopePtr& Scope() const noexcept
    {
        return scope_;
    }

protected:
    IAliasRegistry::Ptr registry_;
    AliasScopePtr scope_;
};

META_END_NAMESPACE()

#endif
