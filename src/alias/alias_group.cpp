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

#include "alias_group.h"

#include "alias_registry.h"

META_BEGIN_NAMESPACE()

AliasGroup::AliasGroup(
    BASE_NS::string path, const IAliasRegistry::Ptr& owner, AliasRegistry& registry, AliasScopePtr scope)
    : path_(BASE_NS::move(path)), owner_(owner), registry_(&registry), scope_(BASE_NS::move(scope))
{}

BASE_NS::string AliasGroup::GetPath() const
{
    return path_;
}

bool AliasGroup::Set(BASE_NS::array_view<const AliasEntry> values)
{
    auto owner = owner_.lock();
    return owner && registry_->GroupSet(path_, values, scope_);
}

bool AliasGroup::Define(BASE_NS::string_view name, const META_NS::IAny& value)
{
    auto owner = owner_.lock();
    // A single-value call states no transition, so it inherits the group's; Set is the call that carries
    // a fully described entry per value.
    return owner && registry_->GroupContribute(path_, name, &value, registry_->GroupTransition(path_, scope_), scope_);
}

bool AliasGroup::Define(BASE_NS::string_view name, const META_NS::IAny& value, const TransitionSpec& transition)
{
    auto owner = owner_.lock();
    return owner && registry_->GroupContribute(path_, name, &value, transition, scope_);
}

bool AliasGroup::Mask(BASE_NS::string_view name)
{
    auto owner = owner_.lock();
    return owner && registry_->GroupContribute(path_, name, nullptr, registry_->GroupTransition(path_, scope_), scope_);
}

bool AliasGroup::Undefine(BASE_NS::string_view name)
{
    auto owner = owner_.lock();
    return owner && registry_->GroupUndefine(path_, name, scope_);
}

BASE_NS::vector<BASE_NS::string> AliasGroup::GetNames() const
{
    auto owner = owner_.lock();
    return owner ? registry_->GroupNames(path_, scope_) : BASE_NS::vector<BASE_NS::string>{};
}

void AliasGroup::SetTransition(const TransitionSpec& transition)
{
    if (auto owner = owner_.lock()) {
        registry_->GroupSetTransition(path_, transition, scope_);
    }
}

TransitionSpec AliasGroup::GetTransition() const
{
    auto owner = owner_.lock();
    return owner ? registry_->GroupTransition(path_, scope_) : TransitionSpec{};
}

void AliasGroup::Clear()
{
    if (auto owner = owner_.lock()) {
        // Only this layer: groups nested under it are their own layers, and RemoveGroup is what takes
        // a whole subtree.
        registry_->GroupSet(path_, {}, scope_);
    }
}

META_END_NAMESPACE()
