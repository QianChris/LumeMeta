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

#ifndef META_INTERFACE_ALIAS_IALIAS_REGISTRY_H
#define META_INTERFACE_ALIAS_IALIAS_REGISTRY_H

#include <meta/base/namespace.h>
#include <meta/interface/alias/intf_alias_group.h>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>

#include <meta/base/interface_macros.h>
#include <meta/base/types.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The token identifying an alias scope — a scene, in practice.
 *
 * Opaque: the registry never calls through it, only compares identity, so a scope can be anything the
 * caller finds meaningful. Deliberately the same shape as a resource's context, since the two answer the
 * same question about the same objects, and the registry holds it weakly.
 */
using AliasScopePtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;

/**
 * @brief What the registry knows about one alias.
 */
struct AliasInfo {
    /// The alias name.
    BASE_NS::string name;
    /// The type its consumers use, invalid when nothing knows it yet — see IAliasRegistry::GetAliasType.
    TypeId type;
    /// The group it was declared in, empty when declared ungrouped or not declared at all.
    BASE_NS::string group;
    /// Whether the alias has a cell, i.e. can hold a value. False for one that is only declared.
    bool hasCell{};
};

/**
 * @brief What happened to an alias.
 */
enum class AliasChangeType : uint8_t {
    /// A cell now exists for the alias, so it can hold a value.
    ADDED,
    /// Its cell is gone; consumers have fallen back to their own values.
    REMOVED,
    /// Resolution ran for it — a different layer decides it now, or the deciding layer's value changed.
    RESOLVED,
    /// Its declaration was stated or withdrawn.
    DECLARED,
};

/**
 * @brief What changed in a registry, for one scope. Carries no value: a listener that wants one reads
 *        it back, so the event cannot go stale.
 * @note  @c names borrows the caller's storage and is valid for the duration of the call only.
 */
struct AliasChangedInfo {
    /// What happened.
    AliasChangeType type;
    /// Whose aliases; null is the global scope.
    AliasScopePtr scope;
    /// The aliases it happened to.
    BASE_NS::array_view<const BASE_NS::string> names;
};

/**
 * @brief Interface for observing what a registry holds.
 */
class IAliasListener : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IAliasListener, "c09dc88e-624a-45d7-830f-15839c6269fb")
public:
    /**
     * @brief Called when aliases change, from whichever thread called the modifying function. No lock is
     *        held, so reading the registry back from here is fine.
     */
    virtual void OnAliasChanged(const AliasChangedInfo& info) = 0;
};

/**
 * @brief Generic named value/resource indirection registry.
 *
 * Consumers bind their properties to a named alias and read them; providers define/change what a name
 * means at runtime. Each alias is backed by a single registry-owned "cell" property; binding makes a
 * consumer's target follow that cell, so when a provider later changes the alias value every bound
 * consumer updates automatically.
 *
 * The mechanism is value-generic: the cell's type is fixed by the first reference (a Bind, a
 * SetDefault or a Define) and works for any property type (resource pointers, colors, floats, ...).
 *
 * A value can be supplied directly, or through an IAliasGroup — a removable layer, so a set of values
 * can be applied and later dropped as a unit. An alias resolves to the topmost layer that supplies a
 * value for it:
 *
 *     Define              ungrouped, overrides every group
 *     IAliasGroup         groups, deepest path wins ("theme/accent" over "theme")
 *     SetDefault          the dormant fallback
 *
 * Argument order follows what a call acts on: operations on an alias take the name first
 * (Define/SetDefault/Undefine/Remove/GetAlias/GetConsumerCount), operations on a consumer property
 * take that property first (Bind/Unbind), so a bind reads as "bind this target to that name".
 *
 * ### Scopes
 *
 * Every call takes a @c scope, an opaque token — a scene, typically — that says which world the call
 * belongs to. Each also has an overload without it, which is the global scope: a caller with no scope in
 * hand says so by not passing one, rather than by passing an empty something. The virtual is the one
 * that takes the scope, so an implementation states what it does with it.
 *
 * A scope does not partition the registry: it layers on top of it. An alias in some scope resolves to
 * whatever that scope supplies, and when it supplies nothing it reads through to the global alias of
 * the same name. So a scene can override one alias and inherit every other, which is the point —
 * scoping by substituting a whole registry could only ever be all or nothing.
 *
 * That read-through is live: a global value changing reaches every scope inheriting it. It applies only
 * where the types agree, since a scope's alias is typed by its own first reference like any other; an
 * alias whose global namesake holds an incompatible type simply does not inherit.
 *
 * The resolution order for one alias in one scope is therefore:
 *
 *     Define              ungrouped, in this scope
 *     IAliasGroup         groups in this scope, deepest path first
 *     the global alias    what the same name resolves to at global scope
 *     SetDefault          the dormant fallback
 *
 * A masking layer (IAliasGroup::Mask) decides the alias as much as one supplying a value does, so the
 * walk stops there: a scope masking a name does not inherit the global one.
 *
 * ### Threading
 *
 * A registry has two halves, and they answer differently.
 *
 * Its own bookkeeping — which aliases exist, who consumes them, what each layer supplies — is internally
 * locked and may be reached from any thread.
 *
 * Applying a value is not, because it ends in writing properties, and a property in this codebase belongs
 * to one thread at a time. So the registry is given the queue that owns them, through @ref SetAliasQueue,
 * and every call that can change a value runs its property work there: directly when the caller is
 * already on that thread, and otherwise posted to it and waited for, so a call still means what it says
 * when it returns. Fan-out and transitions land on the same queue, so consumers are only ever written
 * from the thread that owns them.
 *
 * Without a queue every call does its work where it was made, which is right for a registry used from one
 * thread and is what a caller that never mentions queues gets.
 */
class IAliasRegistry : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IAliasRegistry, "92d0ee69-09c4-4fa9-888d-ee404639435d")
public:
    /**
     * @brief Make @p target follow the alias @p name. The alias's value type is taken from @p target
     *        (the cell is created dormant on first reference). Returns the cell property, or null on a
     *        type conflict with an already-established alias of the same name; on failure @p target is
     *        left unbound. Valid to call before the alias is defined.
     *
     *        A target follows at most one alias: rebinding it to another name detaches it from the
     *        previous one.
     */
    virtual META_NS::IProperty::Ptr Bind(
        const META_NS::IProperty::Ptr& target, BASE_NS::string_view name, const AliasScopePtr& scope) = 0;
    /// @overload In the global scope.
    META_NS::IProperty::Ptr Bind(const META_NS::IProperty::Ptr& target, BASE_NS::string_view name)
    {
        return Bind(target, name, {});
    }

    /**
     * @brief Stop @p target following whichever alias it follows; it falls back to its own underlying
     *        value. Returns false if it was not following one.
     */
    virtual bool Unbind(const META_NS::IProperty::Ptr& target) = 0;

    /**
     * @brief Set the default (fallback) value for the alias @p name, creating the cell if it does not
     *        exist yet. Returns the cell property, or null on a type conflict with an
     *        already-established alias of the same name.
     */
    virtual META_NS::IProperty::Ptr SetDefault(
        BASE_NS::string_view name, const META_NS::IAny::Ptr& defaultValue, const AliasScopePtr& scope) = 0;
    /// @overload In the global scope.
    META_NS::IProperty::Ptr SetDefault(BASE_NS::string_view name, const META_NS::IAny::Ptr& defaultValue)
    {
        return SetDefault(name, defaultValue, {});
    }

    /**
     * @brief Set the alias's value to a literal, in the ungrouped layer above every group. Creates the
     *        cell (typed from @p value) if it does not exist yet. Returns false on a type conflict. Fans
     *        out to all bound consumers.
     */
    virtual bool Define(BASE_NS::string_view name, const META_NS::IAny& value, const AliasScopePtr& scope) = 0;
    /// @overload In the global scope.
    bool Define(BASE_NS::string_view name, const META_NS::IAny& value)
    {
        return Define(name, value, {});
    }

    /**
     * @brief Drop the ungrouped value set by @ref Define; the alias falls back to the topmost group that
     *        supplies one, or to its default when no group does. Does NOT destroy the cell or drop
     *        consumer bindings. Returns false if @p name is not an established alias.
     */
    virtual bool Undefine(BASE_NS::string_view name, const AliasScopePtr& scope) = 0;
    /// @overload In the global scope.
    bool Undefine(BASE_NS::string_view name)
    {
        return Undefine(name, {});
    }

    /**
     * @brief Destroy the cell for @p name. Bound consumers fall back to their own underlying value.
     *        Returns false if @p name is not an established alias.
     */
    virtual bool Remove(BASE_NS::string_view name, const AliasScopePtr& scope) = 0;
    /// @overload In the global scope.
    bool Remove(BASE_NS::string_view name)
    {
        return Remove(name, {});
    }

    /**
     * @brief Enumerate what the registry knows about, name and type, for theme tooling and editors.
     *
     *        Covers both established aliases and ones that have only been declared through
     *        @ref SetAliasTypeHint, so an alias a project offers is listed before anything supplies a
     *        value for it. In a scope, the aliases it inherits from the global registry are listed too.
     *        Each entry's type is what @ref GetAliasType would answer.
     */
    virtual BASE_NS::vector<AliasInfo> GetAliases(const AliasScopePtr& scope) const = 0;
    /// @overload In the global scope.
    BASE_NS::vector<AliasInfo> GetAliases() const
    {
        return GetAliases({});
    }

    /**
     * @brief The type the alias's consumers use, or an invalid TypeId when nothing knows it yet.
     *
     *        Answers from the alias's cell when that has a real type, and from the declared hint
     *        otherwise. A cell always wins over a hint, in this scope or the global one, because it is
     *        the type consumers actually bound: a hint only says what an alias is *meant* to be.
     *
     *        A hint is what covers the two cases a cell cannot: an alias declared before anything gives
     *        it a value, and one whose value is a resource reference that nothing has bound yet — such
     *        a cell is deliberately left type-erased until the first concrete bind narrows it.
     */
    virtual TypeId GetAliasType(BASE_NS::string_view name, const AliasScopePtr& scope) const = 0;
    /// @overload In the global scope.
    TypeId GetAliasType(BASE_NS::string_view name) const
    {
        return GetAliasType(name, {});
    }

    /**
     * @brief Declare that the alias @p name is meant to hold values of type @p type.
     *
     *        A hint and nothing more: it creates no cell, contributes no value, takes part in no
     *        layering, and never changes what a consumer reads. It exists so tooling can offer an alias
     *        — and offer it for the right property — before anything has given it a value. Declaring
     *        a type therefore commits nothing: a later value of a different type is not refused.
     *
     *        Code has the type at hand and needs no vocabulary for this: @c UidFromType<IMaterial::Ptr>().
     *        Re-declaring replaces the previous hint. An empty name or invalid type is rejected.
     *
     *        @p group says which set of declarations this one belongs to — the catalog it came from —
     *        so tooling can present a project's aliases the way they were authored rather than as one
     *        flat list. Empty means ungrouped. It labels the declaration and nothing else: it is not a
     *        layer, and shares nothing with the groups a value is applied to.
     *
     *        A declaration outlives the values that come and go under it, so @ref Remove does not drop
     *        it: what a project offers does not stop being true because a theme was taken away.
     */
    virtual bool SetAliasTypeHint(
        BASE_NS::string_view name, const TypeId& type, BASE_NS::string_view group, const AliasScopePtr& scope) = 0;
    /// @overload In the global scope.
    bool SetAliasTypeHint(BASE_NS::string_view name, const TypeId& type, BASE_NS::string_view group)
    {
        return SetAliasTypeHint(name, type, group, {});
    }
    /// @overload Ungrouped, in the global scope.
    bool SetAliasTypeHint(BASE_NS::string_view name, const TypeId& type)
    {
        return SetAliasTypeHint(name, type, {}, {});
    }

    /**
     * @brief Withdraw the declaration for @p name, returning whether there was one.
     *
     *        The counterpart to @ref SetAliasTypeHint, and what a catalog re-applied without an entry it
     *        used to carry needs: declarations are only ever added otherwise, so an alias a project has
     *        stopped offering would go on being offered. Takes no value with it — an alias that has one
     *        keeps it, and goes on reporting the type it was actually given.
     */
    virtual bool RemoveAliasTypeHint(BASE_NS::string_view name, const AliasScopePtr& scope) = 0;
    /// @overload In the global scope.
    bool RemoveAliasTypeHint(BASE_NS::string_view name)
    {
        return RemoveAliasTypeHint(name, {});
    }

    /**
     * @brief The group at @p path, created if it does not exist. A group costs nothing until it
     *        contributes a value, so this never fails on a well-formed path; a path is a non-empty
     *        sequence of non-empty slash-separated segments.
     */
    virtual IAliasGroup::Ptr GetGroup(BASE_NS::string_view path, const AliasScopePtr& scope) = 0;
    /// @overload In the global scope.
    IAliasGroup::Ptr GetGroup(BASE_NS::string_view path)
    {
        return GetGroup(path, {});
    }

    /**
     * @brief The group at @p path, or null if no group there contributes anything. Does not create.
     */
    virtual IAliasGroup::Ptr FindGroup(BASE_NS::string_view path, const AliasScopePtr& scope) const = 0;
    /// @overload In the global scope.
    IAliasGroup::Ptr FindGroup(BASE_NS::string_view path) const
    {
        return FindGroup(path, {});
    }

    /**
     * @brief The ungrouped layer — everything set through @ref Define — as a group, so that what was
     *        defined directly can be enumerated, replaced and released the same way a theme's values
     *        can. Its path is empty, and it stays above every group however deeply nested.
     *
     *        It is reached through this call rather than by naming it, because an empty path would read
     *        as the shallowest group, i.e. the weakest layer, when this is the strongest. For the same
     *        reason it is not listed by @ref GetContributingGroups and cannot be addressed by @ref FindGroup or
     *        @ref RemoveGroup.
     */
    virtual IAliasGroup::Ptr GetUngrouped(const AliasScopePtr& scope) = 0;
    /// @overload In the global scope.
    IAliasGroup::Ptr GetUngrouped()
    {
        return GetUngrouped({});
    }

    /**
     * @brief Release everything the group at @p path contributes, and everything nested under it —
     *        removing "theme" removes "theme/accent" too. Every alias they decided falls back to the
     *        next layer down. Returns false if nothing was contributed there.
     */
    virtual bool RemoveGroup(BASE_NS::string_view path, const AliasScopePtr& scope) = 0;
    /// @overload In the global scope.
    bool RemoveGroup(BASE_NS::string_view path)
    {
        return RemoveGroup(path, {});
    }

    /**
     * @brief Paths of the groups that currently contribute something, shallowest first.
     */
    virtual BASE_NS::vector<BASE_NS::string> GetContributingGroups(const AliasScopePtr& scope) const = 0;
    /// @overload In the global scope.
    BASE_NS::vector<BASE_NS::string> GetContributingGroups() const
    {
        return GetContributingGroups({});
    }

    /**
     * @brief Paths of the groups that state settings of their own, shallowest first.
     *
     *        A setting is not a contribution: it neither makes a group exist nor goes away when the
     *        group's content is released, and it may be stated for a group nothing has been applied to.
     *        So a group can appear here, in @ref GetContributingGroups, or in both. What a group states
     *        is read with @c GetGroup(path)->GetTransition().
     */
    virtual BASE_NS::vector<BASE_NS::string> GetSettingsGroups(const AliasScopePtr& scope) const = 0;
    /// @overload In the global scope.
    BASE_NS::vector<BASE_NS::string> GetSettingsGroups() const
    {
        return GetSettingsGroups({});
    }

    /**
     * @brief Return the cell (source) property for the alias @p name, or null if it is not an
     *        established alias. The cell exposes the alias's current value and its fixed type; a caller
     *        can clone its type (@c GetValue().Clone(false)) to coerce a new value into that type
     *        before @c Define. Does not create the alias.
     */
    virtual META_NS::IProperty::Ptr GetAlias(BASE_NS::string_view name, const AliasScopePtr& scope) const = 0;
    /// @overload In the global scope.
    META_NS::IProperty::Ptr GetAlias(BASE_NS::string_view name) const
    {
        return GetAlias(name, {});
    }

    /**
     * @brief Number of live consumer bindings for the alias @p name (0 if it is not an established
     *        alias). Consumers are held weakly and destroyed ones are not counted; primarily for
     *        introspection/diagnostics.
     */
    virtual size_t GetConsumerCount(BASE_NS::string_view name, const AliasScopePtr& scope) const = 0;
    /// @overload In the global scope.
    size_t GetConsumerCount(BASE_NS::string_view name) const
    {
        return GetConsumerCount(name, {});
    }

    /**
     * @brief The live consumer properties bound to the alias @p name, empty if it is not an established
     *        alias. Consumers are held weakly, so what comes back is whatever is still alive at the call
     *        — the same set @ref GetConsumerCount counts.
     *
     *        For tooling that has to answer the question the other way round: which of the properties it
     *        is looking at follow an alias, and which one. A caller with many properties to classify
     *        inverts this over @ref GetAliases once rather than asking per property, which is why the
     *        registry offers the consumers instead of a lookup by target. An editor writing a scene file
     *        does exactly that, to recover the bindings the importer created from the file's alias list.
     */
    virtual BASE_NS::vector<META_NS::IProperty::Ptr> GetConsumers(
        BASE_NS::string_view name, const AliasScopePtr& scope) const = 0;
    /// @overload In the global scope.
    BASE_NS::vector<META_NS::IProperty::Ptr> GetConsumers(BASE_NS::string_view name) const
    {
        return GetConsumers(name, {});
    }

    /**
     * @brief Set the task queue the registry applies alias values on — see @ref Threading.
     * @param queueId Id of the queue to use, resolved through the task queue registry when needed, so a
     *        queue registered later than this call still works. @c {} means no queue: every call does its
     *        work on the thread it was made from.
     */
    virtual void SetAliasQueueId(const BASE_NS::Uid& queueId) = 0;
    /**
     * @see SetAliasQueueId
     */
    virtual void SetAliasQueue(const ITaskQueue::Ptr& queue) = 0;

    /**
     * @brief Observe what the registry holds, across every scope. Held **weakly**, as consumers are, so
     *        the caller keeps its own reference and forgetting to remove one leaks nothing. Adding the
     *        same listener twice adds it once.
     */
    virtual void AddListener(const IAliasListener::Ptr& listener) = 0;
    /**
     * @brief Stop observing. Removing one that was never added does nothing. A call already in flight
     *        may still arrive after this returns.
     */
    virtual void RemoveListener(const IAliasListener::Ptr& listener) = 0;
};

META_REGISTER_CLASS(AliasRegistry, "b562ae0f-473d-4116-934b-83aef23db5d0", META_NS::ObjectCategoryBits::NO_CATEGORY)

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IAliasRegistry)

#endif
