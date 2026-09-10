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

#ifndef META_SRC_ALIAS_ALIAS_REGISTRY_H
#define META_SRC_ALIAS_ALIAS_REGISTRY_H

#include <meta/interface/alias/intf_alias_registry.h>

#include <mutex>
#include <shared_mutex>

#include <base/containers/unordered_map.h>

#include <meta/api/property/property_event_handler.h>
#include <meta/ext/object.h>
#include <meta/interface/intf_task_queue_registry.h>

#include "../animation/transition_driver.h"
#include "../run_on_queue.h"

META_BEGIN_NAMESPACE()

/**
 * @brief Generic named value/resource indirection registry.
 *
 * Implements the IAliasRegistry consumer and provider roles over a map of named "cell" properties.
 * Consumers bind their properties to a cell; providers mutate the cell, fanning out to every bound
 * consumer via META property bindings.
 */
class AliasRegistry : public META_NS::IntroduceInterfaces<META_NS::BaseObject, IAliasRegistry> {
    META_OBJECT(AliasRegistry, ClassId::AliasRegistry, IntroduceInterfaces)
public:
    // IAliasRegistry
    META_NS::IProperty::Ptr Bind(
        const META_NS::IProperty::Ptr& target, BASE_NS::string_view name, const AliasScopePtr& scope) override;
    bool Unbind(const META_NS::IProperty::Ptr& target) override;
    META_NS::IProperty::Ptr SetDefault(
        BASE_NS::string_view name, const META_NS::IAny::Ptr& defaultValue, const AliasScopePtr& scope) override;
    bool Define(BASE_NS::string_view name, const META_NS::IAny& value, const AliasScopePtr& scope) override;
    bool Undefine(BASE_NS::string_view name, const AliasScopePtr& scope) override;
    bool Remove(BASE_NS::string_view name, const AliasScopePtr& scope) override;
    BASE_NS::vector<AliasInfo> GetAliases(const AliasScopePtr& scope) const override;
    META_NS::TypeId GetAliasType(BASE_NS::string_view name, const AliasScopePtr& scope) const override;
    bool SetAliasTypeHint(BASE_NS::string_view name, const META_NS::TypeId& type, BASE_NS::string_view group,
        const AliasScopePtr& scope) override;
    bool RemoveAliasTypeHint(BASE_NS::string_view name, const AliasScopePtr& scope) override;
    void AddListener(const IAliasListener::Ptr& listener) override;
    void RemoveListener(const IAliasListener::Ptr& listener) override;
    META_NS::IProperty::Ptr GetAlias(BASE_NS::string_view name, const AliasScopePtr& scope) const override;
    size_t GetConsumerCount(BASE_NS::string_view name, const AliasScopePtr& scope) const override;
    BASE_NS::vector<META_NS::IProperty::Ptr> GetConsumers(
        BASE_NS::string_view name, const AliasScopePtr& scope) const override;
    IAliasGroup::Ptr GetGroup(BASE_NS::string_view path, const AliasScopePtr& scope) override;
    IAliasGroup::Ptr FindGroup(BASE_NS::string_view path, const AliasScopePtr& scope) const override;
    IAliasGroup::Ptr GetUngrouped(const AliasScopePtr& scope) override;
    bool RemoveGroup(BASE_NS::string_view path, const AliasScopePtr& scope) override;
    BASE_NS::vector<BASE_NS::string> GetContributingGroups(const AliasScopePtr& scope) const override;
    BASE_NS::vector<BASE_NS::string> GetSettingsGroups(const AliasScopePtr& scope) const override;
    void SetAliasQueueId(const BASE_NS::Uid& queueId) override;
    void SetAliasQueue(const META_NS::ITaskQueue::Ptr& queue) override;

    // Called by AliasGroup, which is a handle onto a layer rather than a holder of one. `value` null
    // masks the alias in that layer.
    bool GroupContribute(BASE_NS::string_view group, BASE_NS::string_view name, const META_NS::IAny* value,
        const TransitionSpec& transition, const AliasScopePtr& scope);
    bool GroupUndefine(BASE_NS::string_view group, BASE_NS::string_view name, const AliasScopePtr& scope);
    bool GroupSet(BASE_NS::string_view group, BASE_NS::array_view<const AliasEntry> values, const AliasScopePtr& scope);
    BASE_NS::vector<BASE_NS::string> GroupNames(BASE_NS::string_view group, const AliasScopePtr& scope) const;
    void GroupSetTransition(BASE_NS::string_view group, const TransitionSpec& transition, const AliasScopePtr& scope);
    // The transition a value applied to `group` inherits: the setting on that path, or the nearest one
    // above it. Settings live apart from the contributions so that unloading a layer's content does not
    // forget how the layer behaves, and so that one can be stated before anything is applied there.
    TransitionSpec GroupTransition(BASE_NS::string_view group, const AliasScopePtr& scope) const;

private:
    // What one layer supplies for one alias. `group` is the layer's path, empty for the ungrouped layer
    // that Define writes; `value` null is a mask, i.e. the layer states there is no value here and the
    // layers below it do not get to supply one.
    struct Contribution {
        BASE_NS::string group;
        META_NS::IAny::Ptr value;
        // Resolved when the contribution was recorded, so withdrawing it later needs nothing but the
        // contribution itself: a layer leaves the way it came, whatever has been applied since.
        TransitionSpec transition;
    };

    struct AliasCell {
        // The cell (source) property; consumers bind to it, providers mutate it. Never null for an
        // established alias: a cell is only inserted once it has been constructed.
        META_NS::IProperty::Ptr cell;
        // Bound consumer properties, held weakly. Used by Remove to drop the binding so consumers
        // fall back to their own value (a consumer's bind weak-references the cell, so destroying the
        // cell alone leaves the consumer frozen at its last value rather than falling back).
        BASE_NS::vector<META_NS::IProperty::WeakPtr> consumers;
        // Tells the scopes inheriting this alias when the global cell changes. Owned here so it
        // auto-unsubscribes when the cell is removed.
        META_NS::PropertyChangedEventHandler changedHandler;
        // Whether the alias has ever held a value. The first one cannot be transitioned to: a cell that
        // has never been written sits on a type's zero rather than on anything anyone designed, so
        // easing a scale up from (0,0,0) or a colour up from black is not a fade, it is a glitch. Once
        // the alias has held something, falling back to the default and returning are both real changes
        // and both transition.
        bool established{};
    };

    // Everything one scope holds. The global scope is one of these and behaves exactly as the whole
    // registry did before scopes existed; every other scope layers on top of it.
    struct ScopeState {
        BASE_NS::unordered_map<BASE_NS::string, AliasCell> cells;
        // Per alias, what each layer supplies for it, in the order the contributions arrived — which is
        // how two layers of equal depth are settled. The cell is the resolved output of this; keeping
        // the two apart is what lets a layer mask an alias that has no cell yet, so a mask does not
        // depend on the layer it masks having been applied first. Only masks are ever held for a name
        // with no cell: a contribution carrying a value establishes one.
        BASE_NS::unordered_map<BASE_NS::string, BASE_NS::vector<Contribution>> layers;
        // Per-group settings, keyed by path. Deliberately not merged into `layers`: a setting is not a
        // contribution, so it neither makes a group exist nor goes away when the group's content is
        // released, and it can be stated for a group nothing has been applied to yet.
        BASE_NS::unordered_map<BASE_NS::string, TransitionSpec> groupSettings;
        // Declared types, for aliases nothing has valued yet. Kept apart from `cells` because a
        // declaration is not an alias: it holds no value, so giving it a cell would fix the alias to a
        // type a later consumer might not accept, which is exactly what late typing exists to avoid.
        // `group` is the catalog the declaration came from, a label only — declarations do not layer.
        struct Declaration {
            META_NS::TypeId type;
            BASE_NS::string group;
        };
        BASE_NS::unordered_map<BASE_NS::string, Declaration> hints;
    };

    // One non-global scope. `key` is the scope's identity kept as a bare address, so a scope whose owner
    // has been destroyed can still be matched and swept; nothing is ever dereferenced through it.
    struct ScopedState {
        BASE_NS::weak_ptr<CORE_NS::IInterface> scope;
        const void* key{};
        ScopeState state;
    };

    // Forget every scope whose owner is gone. Caller must hold the write lock.
    void DropDeadScopes();

    // The state for `scope`, created if it does not exist. Caller must hold the write lock.
    ScopeState& AcquireScope(const AliasScopePtr& scope);

    // The state for `scope`, or null when nothing has been put in it. Caller must hold either lock.
    ScopeState* FindScope(const AliasScopePtr& scope);
    const ScopeState* FindScope(const AliasScopePtr& scope) const;

    // Re-resolve every scoped alias called `name` that is currently inheriting, because the global one
    // changed. Runs from the global cell's change notification, so with the lock released.
    void PropagateToScopes(BASE_NS::string_view name);

    // The value a scoped alias inherits: what `name` resolves to at global scope, or null when there is
    // no global cell or its type is one this scope's cell cannot take. Caller must hold either lock.
    META_NS::IAny::Ptr InheritedValue(BASE_NS::string_view name, const META_NS::IProperty& cell) const;

    // Get-or-create the cell for `name`, typed from `seed` on first reference. Null on a type conflict
    // with an already-established alias. Wraps the FindCell/CreateCell/EnsureCell sequence, including
    // building the candidate outside the lock.
    META_NS::IProperty::Ptr AcquireCell(
        BASE_NS::string_view name, const META_NS::IAny& seed, const AliasScopePtr& scope);

    // Push the value of the topmost layer contributing to `name` into its cell, or return the cell to
    // its default when the winning layer is a mask and when no layer contributes at all. No-op for a
    // name with no cell: nothing can hold a value yet, and a cell is born dormant, which is already what
    // a mask resolves to. Takes and releases the lock itself; the cell is written outside it.
    //
    // `transition` is the one belonging to whatever caused this resolution — the contribution recorded,
    // or the one withdrawn. Resolution never consults a layer for it, which is what keeps a value moving
    // between layers unambiguous: the change is described by the operation, not by the layer it lands on
    // or leaves.
    bool Resolve(BASE_NS::string_view name, const TransitionSpec& transition, const AliasScopePtr& scope);

    // Record `contribution` for `name`, replacing what the same layer contributed before. A layer that
    // re-contributes keeps its position, so re-applying a value set does not reshuffle how two layers of
    // equal depth are settled. Caller must hold the write lock.
    void RecordContribution(ScopeState& st, BASE_NS::string_view name, Contribution&& contribution);

    // Drop what layer `group` contributes to `name`, returning whether it contributed anything and, when
    // it did, the transition it carried — what the alias falls back with. Caller must hold the write
    // lock.
    bool DropContribution(
        ScopeState& st, BASE_NS::string_view name, BASE_NS::string_view group, TransitionSpec& transition);

    // Move `cell` towards `to` over `transition` rather than writing it outright. False when the change
    // is not one that can be interpolated and the caller should just write it: no transition asked for,
    // no driver, or a type with nothing meaningful between two values. Must be called with the lock
    // released — it builds the driver and writes cells.
    bool Animate(
        const META_NS::IProperty::Ptr& cell, const META_NS::IAny& to, const TransitionSpec& transition, bool toDefault);

    // The driver, built on first use. Must be called with the lock released: constructing it goes out to
    // the object registry.
    ITransitionDriver::Ptr AcquireDriver();

    // Stop any transition running on `cell`, for a cell that is being destroyed or replaced. Never builds
    // a driver: a cell can only be mid-transition if one already exists.
    void CancelTransition(const META_NS::IProperty::Ptr& cell);

    // Note that `name` has now held a value, so that later changes to it are transitions rather than a
    // first appearance. Locks internally.
    void MarkEstablished(BASE_NS::string_view name, const AliasScopePtr& scope);

    // Tell the listeners, with the lock released: a listener may call straight back in. Caller must NOT
    // hold the lock. Batches are one name today; the multi-name operations can grow real ones later.
    void Notify(AliasChangeType type, const AliasScopePtr& scope,
        const BASE_NS::array_view<const BASE_NS::string>& names) const;
    void Notify(AliasChangeType type, const AliasScopePtr& scope, BASE_NS::string_view name) const;

    // The live listeners, held strongly for the duration of the call. Expired slots are skipped, not
    // dropped: reclaiming wants the write lock, so AddListener/RemoveListener do it instead.
    BASE_NS::vector<IAliasListener::Ptr> GetListeners() const;

    // The type `name`'s cell carries in `st`, or invalid when there is no cell or it is still
    // late-typed — an erased cell knows nothing, so it answers like no cell at all. Caller must hold
    // either lock.
    static META_NS::TypeId CellType(const ScopeState& st, BASE_NS::string_view name);

    // What was declared for `name` in `st`, or null. Caller must hold either lock.
    static const ScopeState::Declaration* Declared(const ScopeState& st, BASE_NS::string_view name);

    // The type of `name` as GetAliasType reports it: a cell beats a declaration, and this scope beats
    // the global one. Cells come first across both scopes because a cell is the type consumers actually
    // bound, and a scope inheriting a global alias ends up on that type anyway. Caller must hold either
    // lock.
    META_NS::TypeId ResolveType(BASE_NS::string_view name, const AliasScopePtr& scope) const;

    // The established cell for `name` if it can take `seed`, else null; `conflict` is set when one
    // exists but rejects `seed`. Lets a caller skip building a candidate cell for an alias that already
    // has one — the common case when a value set is applied over a scene whose bindings established the
    // aliases. Advisory only: EnsureCell re-checks under the write lock.
    META_NS::IProperty::Ptr FindCell(
        BASE_NS::string_view name, const META_NS::IAny& seed, bool& conflict, const AliasScopePtr& scope) const;

    // Build a cell of `seed`'s type for the alias `name`, with its change handler already subscribed.
    // Returns a cell with a null `cell` member on failure. Constructs objects and subscribes, so it is
    // called before taking the lock; the result is discarded if the alias turns out to exist already.
    AliasCell CreateCell(BASE_NS::string_view name, const META_NS::IAny& seed, const AliasScopePtr& scope);

    // Get-or-create the cell for `name`, fixing its type from `seed` on first reference by adopting
    // `candidate`. Returns null on a type conflict with an already-established alias of the same name.
    // `created` says whether this call is what made the alias exist, so the caller can say so once the
    // lock is released.
    AliasCell* EnsureCell(
        ScopeState& st, BASE_NS::string_view name, const META_NS::IAny& seed, AliasCell&& candidate, bool& created);

    // True when `cell` holds an erased interface pointer, i.e. the alias was established by a value
    // whose concrete type was not knowable yet (a resolved resource, defined before anything bound the
    // name). Such a cell accepts any interface-pointer reference until a concrete one narrows it.
    static bool IsLateTyped(const META_NS::IProperty& cell);

    // Whether `consumer` still follows an alias, i.e. still carries a bind on top of its stack. Setting
    // a value on a bound property replaces the bind, and nothing reports that.
    static bool StillBound(const META_NS::IProperty::Ptr& consumer);

    // Write `value` into the value under `consumer`'s bind that stands for storage outside the
    // property, so the alias reaches what the property is a view of. Returns whether anything took it.
    static bool PushBelowBind(const META_NS::IProperty::Ptr& consumer, const META_NS::IAny& value);

    // If `name` is late-typed and `seed` is concrete, replace its cell with `candidate` (consuming it),
    // carrying the value over. Consumers that bound while the cell was erased are appended to `repoint`
    // for the caller to rebind. Locks internally, and does its value copy between the lookup and the
    // swap so that nothing external is called while the lock is held. No-op in every other case.
    void NarrowLateTypedCell(BASE_NS::string_view name, const META_NS::IAny& seed, AliasCell& candidate,
        BASE_NS::vector<META_NS::IProperty::Ptr>& repoint, const AliasScopePtr& scope);

    // Drop every consumer slot referring to `target`, plus any slot whose weak_ptr has expired. A
    // property follows at most one alias, so a rebind must not leave a slot behind on the previous
    // alias: that alias would keep pushing its value into the property's engine value even though the
    // property no longer follows it. Also keeps the vectors bounded to live bindings. Returns whether
    // `target` was found on any alias.
    bool DetachConsumer(const META_NS::IProperty::Ptr& target);

    // Drop consumer slots whose weak_ptr has expired (their property was destroyed), so the vector
    // stays bounded to live bindings rather than growing on every rebind.
    static void PruneExpiredConsumers(AliasCell& entry);

    // Drop the slots for `drop`, and any expired one met on the way. Caller must hold the write lock.
    static void DropConsumers(AliasCell& entry, const BASE_NS::array_view<const META_NS::IProperty::Ptr>& drop);

private:
    // Guards every scope's tables and consumer lists. Held only across table reads and updates, never
    // across a property mutation: those notify, and a consumer's change handler does arbitrary work that
    // can call straight back into the registry.
    mutable std::shared_mutex mutex_;

    // The queue alias values are applied on, or null when there is none. Resolved from the id on first
    // use, so a queue registered after SetAliasQueueId still works.
    META_NS::ITaskQueue::Ptr GetQueue() const;

    // Run `fn` where the properties it touches belong. Everything that writes a property goes through
    // this, and nothing else does.
    template<typename Fn>
    void OnAliasQueue(Fn&& fn) const
    {
        RunOnQueueAndWait(GetQueue(), BASE_NS::forward<Fn>(fn));
    }

    // What the global scope holds, and what each other scope holds on top of it.
    ScopeState global_;
    // Scopes other than the global one. A vector rather than a map because a registry has a handful of
    // scopes at most — one per live scene — while it may have hundreds of aliases, so the cost that
    // matters is the per-alias one, which stays a hashed lookup inside the scope.
    BASE_NS::vector<ScopedState> scoped_;

    // Weak, as consumers are: the registry outlives what watches it, so listening must not pin the
    // listener. Pruned by AddListener/RemoveListener, which already hold the write lock.
    BASE_NS::vector<IAliasListener::WeakPtr> listeners_;

    // Interpolates the cells currently mid-transition. Built on the first transition a registry actually
    // runs, so one that never animates never builds one; guarded on its own because it is reached with
    // the registry lock released, which is where every cell write happens.
    mutable std::mutex driverMutex_;
    ITransitionDriver::Ptr driver_;

    // Set once, before the registry is shared, so they are not guarded: an owner that changes the queue
    // under a running registry is asking which thread its properties belong to mid-answer.
    BASE_NS::Uid queueId_;
    mutable META_NS::ITaskQueue::WeakPtr queue_;
};

META_END_NAMESPACE()

#endif
