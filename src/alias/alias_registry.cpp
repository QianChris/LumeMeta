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

#include "alias_registry.h"

#include <algorithm>

#include <meta/api/engine/util.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/detail/property.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/property/intf_stack_property.h>

#include "alias_group.h"

META_BEGIN_NAMESPACE()

namespace {
// Layer priority. Depth is priority, so a group nested in another overrides it; the ungrouped layer
// (an empty path, what Define writes) sits above every group.
size_t GroupDepth(BASE_NS::string_view group)
{
    if (group.empty()) {
        return SIZE_MAX;
    }
    size_t depth = 1;
    for (auto c : group) {
        if (c == '/') {
            ++depth;
        }
    }
    return depth;
}

// Whether `group` is `root` itself or nested under it. Compares whole segments, so "themes" is not
// under "theme".
bool IsUnderGroup(BASE_NS::string_view group, BASE_NS::string_view root)
{
    return group == root ||
           (group.size() > root.size() && group[root.size()] == '/' && group.substr(0, root.size()) == root);
}

// A path is a non-empty sequence of non-empty slash-separated segments.
bool IsValidGroupPath(BASE_NS::string_view path)
{
    if (path.empty() || path[0] == '/' || path[path.size() - 1] == '/') {
        return false;
    }
    for (size_t i = 1; i < path.size(); ++i) {
        if (path[i] == '/' && path[i - 1] == '/') {
            return false;
        }
    }
    return true;
}

// An erased interface pointer: the one any type every interface-pointer any can be read as and
// written from, and so the representation of "an object whose concrete type is not known here".
bool IsErasedPointer(const META_NS::IAny& any)
{
    return any.GetTypeId() == META_NS::SharedPtrIInterfaceId;
}

// Whether an established cell can take `seed`: directly, or because the cell is late-typed and `seed`
// carries an interface pointer. The late-typed check runs this way round because a typed pointer any
// accepts the erased id while the erased any does not list the typed ones.
bool CellAccepts(const META_NS::IProperty& cell, const META_NS::IAny& seed)
{
    return cell.IsCompatible(seed.GetTypeId()) ||
           (IsErasedPointer(cell.GetValue()) && META_NS::IsCompatible(seed, META_NS::SharedPtrIInterfaceId));
}
}  // namespace

bool AliasRegistry::IsLateTyped(const META_NS::IProperty& cell)
{
    return IsErasedPointer(cell.GetValue());
}

bool AliasRegistry::PushBelowBind(const META_NS::IProperty::Ptr& consumer, const META_NS::IAny& value)
{
    auto stack = interface_cast<META_NS::IStackProperty>(consumer);
    if (!stack) {
        return false;
    }
    // Top-down past the binds — reaching past them is the point — to the first value that is a window
    // onto storage elsewhere: a non-any IValue, an engine value or a scene's converting value that
    // forwards to one, where writing is how you write to what it stands for. An any stops the walk: it
    // owns the value it holds, and it is what the property reads, so writing under it would leave the
    // two disagreeing.
    auto values = stack->GetValues({}, false);
    for (size_t i = values.size(); i > 0; --i) {
        auto& v = values[i - 1];
        if (interface_cast<META_NS::IBind>(v)) {
            continue;
        }
        if (interface_cast<META_NS::IAny>(v)) {
            return false;
        }
        META_NS::InterfaceUniqueLock lock{v};
        return static_cast<bool>(v->SetValue(value));
    }
    return false;
}

bool AliasRegistry::StillBound(const META_NS::IProperty::Ptr& consumer)
{
    // A bind is a value, so setting one replaces it and the consumer stops following the alias. Nothing
    // tells the registry, so ask the property: our bind is either on top of its stack or gone, since
    // SetBind pushes on top and a set pops from the top down until something accepts.
    //
    // This asks whether *a* bind is there, not whether it is ours: alias binds are the stock META Bind.
    // Nothing binds a scene property to anything else today, so the distinction has not been worth its
    // own bind class.
    auto stack = interface_cast<META_NS::IStackProperty>(consumer);
    if (!stack) {
        return false;
    }
    auto top = stack->TopValue();
    return interface_cast<META_NS::IBind>(top) != nullptr;
}

void AliasRegistry::DropDeadScopes()
{
    // A dead scope's cells have no consumers left — the properties that bound them belonged to the
    // object that has just gone — so nothing is notified by letting them go here.
    for (auto it = scoped_.begin(); it != scoped_.end();) {
        it = it->scope.expired() ? scoped_.erase(it) : it + 1;
    }
}

AliasRegistry::ScopeState& AliasRegistry::AcquireScope(const AliasScopePtr& scope)
{
    if (!scope) {
        return global_;
    }
    // Before anything is keyed by address: an allocator hands the address of a destroyed scope straight
    // back out, so a stale entry left behind would silently become the new scope's alias state.
    DropDeadScopes();
    const void* key = scope.get();
    for (auto& s : scoped_) {
        if (s.key == key) {
            return s.state;
        }
    }
    // Filled in place rather than aggregate-initialised: a scope's cells own their change handlers, which
    // are not copyable.
    scoped_.push_back({});
    auto& added = scoped_.back();
    added.scope = scope;
    added.key = key;
    return added.state;
}

AliasRegistry::ScopeState* AliasRegistry::FindScope(const AliasScopePtr& scope)
{
    if (!scope) {
        return &global_;
    }
    const void* key = scope.get();
    for (auto& s : scoped_) {
        // A dead scope that happens to share this address is not this scope. It is left in place — this
        // runs under a shared lock and is also the const path — and AcquireScope clears it out.
        if (s.key == key && !s.scope.expired()) {
            return &s.state;
        }
    }
    return nullptr;
}

const AliasRegistry::ScopeState* AliasRegistry::FindScope(const AliasScopePtr& scope) const
{
    return const_cast<AliasRegistry*>(this)->FindScope(scope);
}

META_NS::IAny::Ptr AliasRegistry::InheritedValue(BASE_NS::string_view name, const META_NS::IProperty& cell) const
{
    auto it = global_.cells.find(name);
    if (it == global_.cells.end() || !it->second.cell) {
        return nullptr;
    }
    const auto& value = it->second.cell->GetValue();
    if (cell.IsCompatible(value.GetTypeId())) {
        return value.Clone(true);
    }
    // The two were typed by different first references — a float consumer in this scope, a document's
    // bare number globally — and any compatibility is asymmetric, so the global value can be one this
    // cell will not take as it stands. Hand it one of its own type, as a value set does when it meets an
    // established alias.
    if (auto converted = cell.GetValue().Clone(false); converted && converted->CopyFrom(value)) {
        return converted;
    }
    // Inheritance is a fallback, not a contract: where the types genuinely disagree the scope keeps its
    // own default rather than the whole resolution failing.
    return nullptr;
}

META_NS::IProperty::Ptr AliasRegistry::FindCell(
    BASE_NS::string_view name, const META_NS::IAny& seed, bool& conflict, const AliasScopePtr& scope) const
{
    std::shared_lock guard{mutex_};
    auto st = FindScope(scope);
    if (!st) {
        return nullptr;
    }
    auto it = st->cells.find(name);
    if (it == st->cells.end()) {
        return nullptr;
    }
    if (!CellAccepts(*it->second.cell, seed)) {
        conflict = true;
        return nullptr;
    }
    return it->second.cell;
}

AliasRegistry::AliasCell AliasRegistry::CreateCell(
    BASE_NS::string_view name, const META_NS::IAny& seed, const AliasScopePtr& scope)
{
    AliasCell entry;
    BASE_NS::string key(name);
    // The cell carries a type-only (default-constructed) value, so it starts dormant regardless of
    // which role created it.
    auto typeOnly = seed.Clone(false);
    if (!typeOnly) {
        return entry;
    }
    auto cell = META_NS::ConstructPropertyAny(key, *typeOnly, META_NS::ObjectFlagBits::INTERNAL);
    if (!cell) {
        return entry;
    }
    // Consumers follow the cell through their own binds, and carry what they read down to whatever
    // storage they stand for. What that leaves for the registry is the global cell's other duty:
    // telling the scopes that inherit it.
    const bool isGlobal = !scope;
    entry.changedHandler.Subscribe(cell, [this, key, isGlobal] {
        if (isGlobal) {
            PropagateToScopes(key);
        }
    });
    entry.cell = BASE_NS::move(cell);
    return entry;
}

AliasRegistry::AliasCell* AliasRegistry::EnsureCell(
    ScopeState& st, BASE_NS::string_view name, const META_NS::IAny& seed, AliasCell&& candidate, bool& created)
{
    created = false;
    if (auto it = st.cells.find(name); it != st.cells.end()) {
        return CellAccepts(*it->second.cell, seed) ? &it->second : nullptr;
    }
    if (!candidate.cell) {
        return nullptr;
    }
    // First reference fixes the type.
    created = true;
    return &(st.cells[name] = BASE_NS::move(candidate));
}

void AliasRegistry::DropConsumers(AliasCell& entry, const BASE_NS::array_view<const META_NS::IProperty::Ptr>& drop)
{
    for (auto it = entry.consumers.begin(); it != entry.consumers.end();) {
        auto consumer = it->lock();
        bool dropped = !consumer;
        for (auto& d : drop) {
            dropped = dropped || consumer == d;
        }
        it = dropped ? entry.consumers.erase(it) : it + 1;
    }
}

void AliasRegistry::PruneExpiredConsumers(AliasCell& entry)
{
    auto& consumers = entry.consumers;
    for (auto it = consumers.begin(); it != consumers.end();) {
        if (!it->lock()) {
            it = consumers.erase(it);
        } else {
            ++it;
        }
    }
}

void AliasRegistry::NarrowLateTypedCell(BASE_NS::string_view name, const META_NS::IAny& seed, AliasCell& candidate,
    BASE_NS::vector<META_NS::IProperty::Ptr>& repoint, const AliasScopePtr& scope)
{
    // Only an interface-pointer reference narrows a late-typed cell; anything else is a type conflict
    // and is rejected when the cell is looked up.
    if (IsErasedPointer(seed) || !candidate.cell || !META_NS::IsCompatible(seed, META_NS::SharedPtrIInterfaceId)) {
        return;
    }
    META_NS::IProperty::Ptr erased;
    {
        std::shared_lock guard{mutex_};
        auto st = FindScope(scope);
        if (!st) {
            return;
        }
        auto it = st->cells.find(name);
        if (it == st->cells.end() || !IsLateTyped(*it->second.cell)) {
            return;
        }
        erased = it->second.cell;
    }
    // Interface pointers are stored canonically, so copying across pointer any types preserves the
    // object; a consumer whose interface it does not implement simply reads null.
    OnAliasQueue([&] { META_NS::TypelessPropertyInterface{candidate.cell.get()}.SetValueAny(erased->GetValue()); });
    {
        std::unique_lock guard{mutex_};
        auto st = FindScope(scope);
        if (!st) {
            return;
        }
        auto it = st->cells.find(name);
        if (it == st->cells.end() || it->second.cell != erased) {
            return;
        }
        auto& entry = it->second;
        // Anything that bound while the cell was still erased is re-pointed at the new cell by the
        // caller, once it has let go of the lock.
        for (auto& weak : entry.consumers) {
            if (auto consumer = weak.lock()) {
                repoint.push_back(consumer);
            }
        }
        entry.cell = BASE_NS::move(candidate.cell);
        entry.changedHandler = BASE_NS::move(candidate.changedHandler);
    }
    // Whatever the erased cell was moving towards is meaningless now: it held a pointer any of a type
    // nothing binds any more, and the property it was writing has been replaced.
    CancelTransition(erased);
}

bool AliasRegistry::DetachConsumer(const META_NS::IProperty::Ptr& target)
{
    bool found = false;
    // Every scope, not just one: a property follows at most one alias, but nothing says which scope that
    // alias was in, and a slot left behind anywhere would keep pushing into the property's engine value.
    auto sweep = [&found, &target](ScopeState& st) {
        for (auto& e : st.cells) {
            auto& consumers = e.second.consumers;
            for (auto it = consumers.begin(); it != consumers.end();) {
                auto consumer = it->lock();
                if (!consumer || consumer == target) {
                    found = found || consumer != nullptr;
                    it = consumers.erase(it);
                } else {
                    ++it;
                }
            }
        }
    };
    sweep(global_);
    for (auto& s : scoped_) {
        sweep(s.state);
    }
    return found;
}

META_NS::IProperty::Ptr AliasRegistry::Bind(
    const META_NS::IProperty::Ptr& target, BASE_NS::string_view name, const AliasScopePtr& scope)
{
    if (!target) {
        return nullptr;
    }
    auto seed = target->GetValue().Clone(false);
    if (!seed) {
        return nullptr;
    }
    bool conflict = false;
    auto existing = FindCell(name, *seed, conflict, scope);
    if (conflict) {
        return nullptr;
    }
    // A late-typed alias (defined from a resolved resource before anything bound it) takes its
    // concrete type from this first concrete consumer.
    const bool narrowing = existing && IsLateTyped(*existing) && !IsErasedPointer(*seed);
    AliasCell candidate;
    BASE_NS::vector<META_NS::IProperty::Ptr> repoint;
    if (!existing || narrowing) {
        // Built before the lock is taken: constructing a property and subscribing to it are calls out
        // of the registry. It becomes either this alias's first cell or the narrowed replacement.
        candidate = CreateCell(name, *seed, scope);
        NarrowLateTypedCell(name, *seed, candidate, repoint, scope);
    }

    META_NS::IProperty::Ptr cell;
    bool fresh = false;
    bool created = false;
    {
        std::unique_lock guard{mutex_};
        auto& st = AcquireScope(scope);
        fresh = !existing && st.cells.find(name) == st.cells.end();
        auto entry = EnsureCell(st, name, *seed, BASE_NS::move(candidate), created);
        if (!entry) {
            return nullptr;
        }
        // SetBind replaces whatever `target` followed before, so drop the slot it held on that alias
        // (or on this one, when rebinding the same pair). This also drops slots left behind by
        // consumers that have since been destroyed, e.g. a node template re-instantiated onto a fresh
        // property.
        DetachConsumer(target);
        entry->consumers.push_back(target);
        cell = entry->cell;
    }
    // Outside the lock, and on the queue that owns these properties: binding notifies the target, and a
    // consumer's handler may do anything at all, including instantiating content that binds further
    // properties to this registry.
    bool bound = false;
    OnAliasQueue([&] {
        for (auto& consumer : repoint) {
            META_NS::TypelessPropertyInterface{consumer.get()}.SetBind(cell);
        }
        bound = META_NS::TypelessPropertyInterface{target.get()}.SetBind(cell);
    });
    if (!bound) {
        Unbind(target);
        return nullptr;
    }
    if (created) {
        Notify(AliasChangeType::ADDED, scope, name);
    }
    // A scope's alias starts out inheriting whatever the global one holds, so a consumer that binds in a
    // scope nobody has scoped anything into still sees the shared value rather than a dormant cell.
    if (fresh && scope) {
        Resolve(name, {}, scope);
    }
    return cell;
}

bool AliasRegistry::Unbind(const META_NS::IProperty::Ptr& target)
{
    if (!target) {
        return false;
    }
    {
        std::unique_lock guard{mutex_};
        if (!DetachConsumer(target)) {
            return false;
        }
    }
    // Outside the lock: dropping the bind notifies the target (see Bind).
    OnAliasQueue([&] { META_NS::TypelessPropertyInterface{target.get()}.ResetBind(); });
    return true;
}

META_NS::IProperty::Ptr AliasRegistry::SetDefault(
    BASE_NS::string_view name, const META_NS::IAny::Ptr& defaultValue, const AliasScopePtr& scope)
{
    if (!defaultValue) {
        return nullptr;
    }
    bool conflict = false;
    auto cell = FindCell(name, *defaultValue, conflict, scope);
    if (conflict) {
        return nullptr;
    }
    bool created = false;
    if (!cell) {
        auto candidate = CreateCell(name, *defaultValue, scope);
        {
            std::unique_lock guard{mutex_};
            auto entry = EnsureCell(AcquireScope(scope), name, *defaultValue, BASE_NS::move(candidate), created);
            if (!entry) {
                return nullptr;
            }
            cell = entry->cell;
        }
        if (created) {
            Notify(AliasChangeType::ADDED, scope, name);
        }
    }
    // Outside the lock, and on the queue that owns the properties: setting the default writes the cell a
    // dormant alias reads from, and so notifies its consumers (see Define).
    OnAliasQueue([&] {
        if (META_NS::TypelessPropertyInterface{cell.get()}.SetDefaultValueAny(*defaultValue)) {
            // A default someone stated is a value the alias holds, so the next one to arrive is a change
            // and transitions. Only the type's own zero — what a cell sits on before anyone says anything
            // — is not worth easing away from.
            MarkEstablished(name, scope);
        }
    });
    return cell;
}

META_NS::IProperty::Ptr AliasRegistry::AcquireCell(
    BASE_NS::string_view name, const META_NS::IAny& seed, const AliasScopePtr& scope)
{
    bool conflict = false;
    auto cell = FindCell(name, seed, conflict, scope);
    if (conflict) {
        return nullptr;
    }
    if (cell) {
        return cell;
    }
    // Built before the lock is taken: constructing a property and subscribing to it are calls out of
    // the registry.
    auto candidate = CreateCell(name, seed, scope);
    bool created = false;
    {
        std::unique_lock guard{mutex_};
        auto entry = EnsureCell(AcquireScope(scope), name, seed, BASE_NS::move(candidate), created);
        if (!entry) {
            return nullptr;
        }
        cell = entry->cell;
    }
    if (created) {
        Notify(AliasChangeType::ADDED, scope, name);
    }
    return cell;
}

void AliasRegistry::RecordContribution(ScopeState& st, BASE_NS::string_view name, Contribution&& contribution)
{
    auto& list = st.layers[name];
    for (auto& c : list) {
        if (c.group == contribution.group) {
            c.value = BASE_NS::move(contribution.value);
            return;
        }
    }
    list.push_back(BASE_NS::move(contribution));
}

bool AliasRegistry::DropContribution(
    ScopeState& st, BASE_NS::string_view name, BASE_NS::string_view group, TransitionSpec& transition)
{
    auto it = st.layers.find(name);
    if (it == st.layers.end()) {
        return false;
    }
    auto& list = it->second;
    for (auto c = list.begin(); c != list.end(); ++c) {
        if (c->group == group) {
            transition = c->transition;
            list.erase(c);
            if (list.empty()) {
                st.layers.erase(it);
            }
            return true;
        }
    }
    return false;
}

void AliasRegistry::SetAliasQueueId(const BASE_NS::Uid& queueId)
{
    queue_.reset();
    queueId_ = queueId;
}

void AliasRegistry::SetAliasQueue(const META_NS::ITaskQueue::Ptr& queue)
{
    queueId_ = {};
    queue_ = queue;
}

META_NS::ITaskQueue::Ptr AliasRegistry::GetQueue() const
{
    auto queue = queue_.lock();
    if (!queue && queueId_ != BASE_NS::Uid{}) {
        // Resolved late and remembered weakly: the queue is registered by whoever owns it, which may
        // happen after the registry was told which one to use.
        queue = META_NS::GetTaskQueueRegistry().GetTaskQueue(queueId_);
        queue_ = queue;
    }
    return queue;
}

ITransitionDriver::Ptr AliasRegistry::AcquireDriver()
{
    std::unique_lock guard{driverMutex_};
    if (!driver_) {
        driver_ = interface_pointer_cast<ITransitionDriver>(
            META_NS::GetObjectRegistry().Create<META_NS::IObject>(ClassId::TransitionDriver));
        // The driver eases the same cells, so its writes belong on the same queue: it is stepped by the
        // animation controller, on whatever thread that runs.
        if (driver_) {
            driver_->SetQueue(GetQueue());
        }
    }
    return driver_;
}

bool AliasRegistry::Animate(
    const META_NS::IProperty::Ptr& cell, const META_NS::IAny& to, const TransitionSpec& transition, bool toDefault)
{
    if (transition.duration <= META_NS::TimeSpan::Zero()) {
        return false;
    }
    auto driver = AcquireDriver();
    return driver && driver->Start(cell, to, transition, toDefault);
}

void AliasRegistry::CancelTransition(const META_NS::IProperty::Ptr& cell)
{
    if (!cell) {
        return;
    }
    ITransitionDriver::Ptr driver;
    {
        std::unique_lock guard{driverMutex_};
        driver = driver_;
    }
    if (driver) {
        // Never builds one: a cell can only be mid-transition if a driver already exists.
        driver->Cancel(*cell);
    }
}

void AliasRegistry::MarkEstablished(BASE_NS::string_view name, const AliasScopePtr& scope)
{
    std::unique_lock guard{mutex_};
    auto st = FindScope(scope);
    if (!st) {
        return;
    }
    if (auto it = st->cells.find(name); it != st->cells.end()) {
        it->second.established = true;
    }
}

void AliasRegistry::PropagateToScopes(BASE_NS::string_view name)
{
    // Which scopes hold a cell for this name at all; the ones inheriting will pick the new global value
    // up when they re-resolve, and the ones supplying their own will keep it.
    BASE_NS::vector<AliasScopePtr> scopes;
    {
        std::shared_lock guard{mutex_};
        for (auto& s : scoped_) {
            if (s.state.cells.find(name) != s.state.cells.end()) {
                if (auto locked = s.scope.lock()) {
                    scopes.push_back(BASE_NS::move(locked));
                }
            }
        }
    }
    // Outside the lock, as every resolution is: each one writes a cell and fans out from there.
    for (auto& scope : scopes) {
        Resolve(name, {}, scope);
    }
}

bool AliasRegistry::Resolve(BASE_NS::string_view name, const TransitionSpec& transition, const AliasScopePtr& scope)
{
    META_NS::IProperty::Ptr cell;
    META_NS::IAny::Ptr value;
    bool established = false;
    // A masking layer and no layer at all both leave `value` null, and only the second reads through to
    // the global alias, so which it was has to come out of the walk.
    bool masked = false;
    {
        std::shared_lock guard{mutex_};
        auto st = FindScope(scope);
        if (!st) {
            return true;
        }
        auto ci = st->cells.find(name);
        if (ci == st->cells.end()) {
            // Nothing can hold a value yet. A cell is born dormant, which is already what an alias with
            // no contribution — or with a mask on top — resolves to, so there is nothing to catch up on
            // when one appears.
            return true;
        }
        cell = ci->second.cell;
        established = ci->second.established;
        if (auto li = st->layers.find(name); li != st->layers.end()) {
            size_t winner = 0;
            for (auto& c : li->second) {
                // Deepest layer wins; among layers of equal depth the last contribution applied does,
                // which is the order they are held in. A mask wins as null, leaving the cell dormant.
                if (auto depth = GroupDepth(c.group); depth >= winner) {
                    winner = depth;
                    value = c.value;
                    masked = !c.value;
                }
            }
        }
    }
    // Outside the lock: the fan-out runs every consumer's change handler, and those do arbitrary work
    // — an alias-driven node instantiates a whole subtree, which binds that content's own properties
    // back into this registry. Holding the lock across that would serialise the registry against
    // whatever thread the instantiation lands on.
    // An alias that has never held a value cannot be moved towards one — there is nothing to move from
    // but a type's zero — so a first value always appears at once, however it was applied.
    if (!value && !masked && scope) {
        // Nothing in this scope supplies it, so it reads through to the global alias of the same name.
        // Only when that has nothing either does the alias fall to its own default. A mask supplies the
        // answer "no value here", so it is not nothing and does not read through.
        std::shared_lock guard{mutex_};
        value = InheritedValue(name, *cell);
    }
    // On the queue that owns the properties, from here down: everything below writes the cell and fans
    // out to consumers, and a call made from another thread is waited for, so `res` still means what it
    // says by the time this returns.
    bool ok = true;
    OnAliasQueue([&] {
        if (!value) {
            // Falling back is a change like any other, so it transitions on the way down too: the cell
            // eases towards its default and only lands on dormancy when it arrives.
            if (auto stack = established ? interface_cast<META_NS::IStackProperty>(cell) : nullptr;
                stack && Animate(cell, stack->GetDefaultValue(), transition, true)) {
                return;
            }
            cell->ResetValue();
            return;
        }
        if (established && Animate(cell, *value, transition, false)) {
            return;
        }
        auto res = META_NS::TypelessPropertyInterface{cell.get()}.SetValueAny(*value);
        if (res && !established) {
            MarkEstablished(name, scope);
        }
        ok = bool(res);
    });
    // Raised whether or not the value moved: which layer decides it can change on its own.
    Notify(AliasChangeType::RESOLVED, scope, name);
    return ok;
}

bool AliasRegistry::Define(BASE_NS::string_view name, const META_NS::IAny& value, const AliasScopePtr& scope)
{
    // The ungrouped layer is a layer like any other, so a Define states no transition of its own and
    // takes whatever GetUngrouped() was given — nothing, for a caller that has never asked for one.
    return GroupContribute({}, name, &value, GroupTransition({}, scope), scope);
}

bool AliasRegistry::Undefine(BASE_NS::string_view name, const AliasScopePtr& scope)
{
    TransitionSpec transition;
    {
        std::unique_lock guard{mutex_};
        auto st = FindScope(scope);
        if (!st || st->cells.find(name) == st->cells.end()) {
            return false;
        }
        // Only the ungrouped layer: what the groups contribute is theirs to withdraw, and the alias
        // falls back onto the topmost of them.
        DropContribution(*st, name, {}, transition);
    }
    return Resolve(name, transition, scope);
}

bool AliasRegistry::Remove(BASE_NS::string_view name, const AliasScopePtr& scope)
{
    BASE_NS::vector<META_NS::IProperty::WeakPtr> consumers;
    META_NS::IProperty::Ptr cell;
    {
        std::unique_lock guard{mutex_};
        auto st = FindScope(scope);
        if (!st) {
            return false;
        }
        auto it = st->cells.find(name);
        if (it == st->cells.end()) {
            return false;
        }
        // Destroy the cell first, then drop each consumer's binding so it falls back to its own
        // underlying value. A notification must not find a half-removed alias.
        consumers = BASE_NS::move(it->second.consumers);
        cell = BASE_NS::move(it->second.cell);
        st->cells.erase(it);
        // The alias is gone, not merely without a value, so every layer's contribution to it goes too —
        // otherwise a later Bind would resurrect the old value out of a theme nobody re-applied.
        st->layers.erase(name);
    }
    // Outside the lock: dropping the binds notifies the consumers (see Bind).
    CancelTransition(cell);
    OnAliasQueue([&] {
        for (auto& weak : consumers) {
            if (auto consumer = weak.lock()) {
                META_NS::TypelessPropertyInterface{consumer.get()}.ResetBind();
            }
        }
    });
    Notify(AliasChangeType::REMOVED, scope, name);
    if (!scope) {
        // Every other mutation reaches the scopes inheriting this alias through the cell's change
        // notification. Removal destroys the cell instead of writing it, so nothing is raised and the
        // scopes have to be told by hand — otherwise they keep the value they last inherited for good.
        PropagateToScopes(name);
    }
    return true;
}

META_NS::TypeId AliasRegistry::CellType(const ScopeState& st, BASE_NS::string_view name)
{
    auto it = st.cells.find(name);
    if (it == st.cells.end() || IsLateTyped(*it->second.cell)) {
        return {};
    }
    return it->second.cell->GetValue().GetTypeId();
}

const AliasRegistry::ScopeState::Declaration* AliasRegistry::Declared(const ScopeState& st, BASE_NS::string_view name)
{
    auto it = st.hints.find(name);
    return it != st.hints.end() ? &it->second : nullptr;
}

META_NS::TypeId AliasRegistry::ResolveType(BASE_NS::string_view name, const AliasScopePtr& scope) const
{
    const ScopeState* self = FindScope(scope);
    // Null when `scope` already is the global scope, so each step below simply falls through.
    const ScopeState* global = scope ? FindScope({}) : nullptr;
    for (auto* st : {self, global}) {
        if (st) {
            if (auto type = CellType(*st, name); type.IsValid()) {
                return type;
            }
        }
    }
    for (auto* st : {self, global}) {
        if (st) {
            if (auto* declared = Declared(*st, name); declared && declared->type.IsValid()) {
                return declared->type;
            }
        }
    }
    return {};
}

BASE_NS::vector<AliasInfo> AliasRegistry::GetAliases(const AliasScopePtr& scope) const
{
    std::shared_lock guard{mutex_};
    BASE_NS::vector<AliasInfo> aliases;
    const ScopeState* self = FindScope(scope);
    const ScopeState* global = scope ? FindScope({}) : nullptr;
    // A name reached through more than one of these is still one alias, so the first entry made for it
    // stands; every entry's type comes from the same resolution regardless of which state it was found
    // in, so which one found it changes nothing but `hasCell`.
    auto add = [&](BASE_NS::string_view name, bool hasCell) {
        for (auto& a : aliases) {
            if (a.name == name) {
                a.hasCell = a.hasCell || hasCell;
                return;
            }
        }
        // The declaration nearest this scope names the catalog, on the same walk order the type takes.
        BASE_NS::string group;
        for (auto* st : {self, global}) {
            if (auto* declared = st ? Declared(*st, name) : nullptr) {
                group = declared->group;
                break;
            }
        }
        aliases.push_back(AliasInfo{BASE_NS::string(name), ResolveType(name, scope), BASE_NS::move(group), hasCell});
    };
    for (auto* st : {self, global}) {
        if (!st) {
            continue;
        }
        for (auto& e : st->cells) {
            add(e.first, true);
        }
        for (auto& e : st->hints) {
            add(e.first, false);
        }
    }
    return aliases;
}

META_NS::TypeId AliasRegistry::GetAliasType(BASE_NS::string_view name, const AliasScopePtr& scope) const
{
    std::shared_lock guard{mutex_};
    return ResolveType(name, scope);
}

bool AliasRegistry::SetAliasTypeHint(
    BASE_NS::string_view name, const META_NS::TypeId& type, BASE_NS::string_view group, const AliasScopePtr& scope)
{
    if (name.empty() || !type.IsValid()) {
        return false;
    }
    // No consumer hears this — a declaration changes no value — but a listener does.
    {
        std::unique_lock guard{mutex_};
        AcquireScope(scope).hints[name] = {type, BASE_NS::string(group)};
    }
    Notify(AliasChangeType::DECLARED, scope, name);
    return true;
}

bool AliasRegistry::RemoveAliasTypeHint(BASE_NS::string_view name, const AliasScopePtr& scope)
{
    bool removed = false;
    {
        std::unique_lock guard{mutex_};
        auto st = FindScope(scope);
        removed = st && st->hints.erase(name) != 0;
    }
    if (removed) {
        Notify(AliasChangeType::DECLARED, scope, name);
    }
    return removed;
}

void AliasRegistry::AddListener(const IAliasListener::Ptr& listener)
{
    if (!listener) {
        return;
    }
    // Checking and adding must be one step, so this cannot be built out of GetListeners.
    std::unique_lock guard{mutex_};
    bool known = false;
    for (auto it = listeners_.begin(); it != listeners_.end();) {
        auto live = it->lock();
        if (!live) {
            it = listeners_.erase(it);
        } else {
            known = known || live == listener;
            ++it;
        }
    }
    if (!known) {
        listeners_.push_back(listener);
    }
}

void AliasRegistry::RemoveListener(const IAliasListener::Ptr& listener)
{
    std::unique_lock guard{mutex_};
    for (auto it = listeners_.begin(); it != listeners_.end();) {
        auto live = it->lock();
        it = (!live || live == listener) ? listeners_.erase(it) : it + 1;
    }
}

BASE_NS::vector<IAliasListener::Ptr> AliasRegistry::GetListeners() const
{
    BASE_NS::vector<IAliasListener::Ptr> listeners;
    std::shared_lock guard{mutex_};
    listeners.reserve(listeners_.size());
    for (auto& weak : listeners_) {
        if (auto live = weak.lock()) {
            listeners.push_back(BASE_NS::move(live));
        }
    }
    return listeners;
}

void AliasRegistry::Notify(
    AliasChangeType type, const AliasScopePtr& scope, const BASE_NS::array_view<const BASE_NS::string>& names) const
{
    auto listeners = GetListeners();
    if (listeners.empty()) {
        return;
    }
    // Outside the lock, as every fan-out here is: a listener may call back into the registry.
    const AliasChangedInfo info{type, scope, names};
    for (auto& listener : listeners) {
        listener->OnAliasChanged(info);
    }
}

void AliasRegistry::Notify(AliasChangeType type, const AliasScopePtr& scope, BASE_NS::string_view name) const
{
    // Materialised: the caller's string_view may point into a table entry this change just erased.
    const BASE_NS::string one(name);
    Notify(type, scope, BASE_NS::array_view<const BASE_NS::string>(&one, 1));
}

META_NS::IProperty::Ptr AliasRegistry::GetAlias(BASE_NS::string_view name, const AliasScopePtr& scope) const
{
    std::shared_lock guard{mutex_};
    auto st = FindScope(scope);
    if (!st) {
        return nullptr;
    }
    if (auto it = st->cells.find(name); it != st->cells.end()) {
        return it->second.cell;
    }
    return nullptr;
}

size_t AliasRegistry::GetConsumerCount(BASE_NS::string_view name, const AliasScopePtr& scope) const
{
    std::shared_lock guard{mutex_};
    auto st = FindScope(scope);
    if (!st) {
        return 0;
    }
    auto it = st->cells.find(name);
    if (it == st->cells.end()) {
        return 0;
    }
    // Count live bindings rather than retained slots: a slot for a destroyed consumer, or for one whose
    // bind has been replaced, is dropped lazily on the next fan-out, and that timing is not part of the
    // contract.
    size_t count = 0;
    for (auto& weak : it->second.consumers) {
        if (auto consumer = weak.lock(); consumer && StillBound(consumer)) {
            ++count;
        }
    }
    return count;
}

BASE_NS::vector<META_NS::IProperty::Ptr> AliasRegistry::GetConsumers(
    BASE_NS::string_view name, const AliasScopePtr& scope) const
{
    std::shared_lock guard{mutex_};
    BASE_NS::vector<META_NS::IProperty::Ptr> consumers;
    auto st = FindScope(scope);
    if (!st) {
        return consumers;
    }
    auto it = st->cells.find(name);
    if (it == st->cells.end()) {
        return consumers;
    }
    // Live bindings only, for the reason GetConsumerCount counts them that way.
    consumers.reserve(it->second.consumers.size());
    for (auto& weak : it->second.consumers) {
        if (auto consumer = weak.lock(); consumer && StillBound(consumer)) {
            consumers.push_back(BASE_NS::move(consumer));
        }
    }
    return consumers;
}

IAliasGroup::Ptr AliasRegistry::GetGroup(BASE_NS::string_view path, const AliasScopePtr& scope)
{
    if (!IsValidGroupPath(path)) {
        return nullptr;
    }
    // A layer costs nothing until something is contributed to it, so there is no group to create here —
    // only a handle addressing the path, in the scope it was asked for.
    return CreateShared<AliasGroup>(BASE_NS::string(path), GetSelf<IAliasRegistry>(), *this, scope);
}

IAliasGroup::Ptr AliasRegistry::FindGroup(BASE_NS::string_view path, const AliasScopePtr& scope) const
{
    if (!IsValidGroupPath(path)) {
        return nullptr;
    }
    {
        std::shared_lock guard{mutex_};
        auto st = FindScope(scope);
        if (!st) {
            return nullptr;
        }
        bool exists = false;
        for (auto it = st->layers.begin(); it != st->layers.end() && !exists; ++it) {
            for (auto& c : it->second) {
                if (c.group == path) {
                    exists = true;
                    break;
                }
            }
        }
        if (!exists) {
            return nullptr;
        }
    }
    // The handle mutates the layer it addresses, so it takes a mutable registry; the lookup that decides
    // whether to hand one out does not.
    auto& self = const_cast<AliasRegistry&>(*this);
    return CreateShared<AliasGroup>(BASE_NS::string(path), GetSelf<IAliasRegistry>(), self, scope);
}

IAliasGroup::Ptr AliasRegistry::GetUngrouped(const AliasScopePtr& scope)
{
    // The one layer whose path is not a path: Define already writes it under the empty key, so a handle
    // on that key reaches exactly the directly defined values, and every group operation means for them
    // what it means for a theme's.
    return CreateShared<AliasGroup>(BASE_NS::string{}, GetSelf<IAliasRegistry>(), *this, scope);
}

bool AliasRegistry::RemoveGroup(BASE_NS::string_view path, const AliasScopePtr& scope)
{
    if (!IsValidGroupPath(path)) {
        return false;
    }
    // Each alias falls back with the transition of the deepest contribution being taken from it — the one
    // that was actually deciding it, and so the one whose departure the change is.
    struct Affected {
        BASE_NS::string name;
        TransitionSpec transition;
    };
    BASE_NS::vector<Affected> affected;
    {
        std::unique_lock guard{mutex_};
        auto st = FindScope(scope);
        if (!st) {
            return false;
        }
        for (auto it = st->layers.begin(); it != st->layers.end();) {
            auto& list = it->second;
            size_t deepest = 0;
            bool removed = false;
            TransitionSpec transition;
            for (auto c = list.begin(); c != list.end();) {
                // The whole subtree: removing "theme" takes "theme/accent" with it.
                if (IsUnderGroup(c->group, path)) {
                    if (const auto depth = GroupDepth(c->group); !removed || depth >= deepest) {
                        deepest = depth;
                        transition = c->transition;
                    }
                    removed = true;
                    c = list.erase(c);
                } else {
                    ++c;
                }
            }
            if (removed) {
                affected.push_back(Affected{it->first, transition});
            }
            if (list.empty()) {
                it = st->layers.erase(it);
            } else {
                ++it;
            }
        }
    }
    // Outside the lock, as every write to a cell is.
    for (auto& entry : affected) {
        Resolve(entry.name, entry.transition, scope);
    }
    return !affected.empty();
}

namespace {
// Shallowest first, and alphabetically within a depth, so a listing reads as the layering does.
void SortByDepth(BASE_NS::vector<BASE_NS::string>& groups)
{
    std::sort(groups.begin(), groups.end(), [](const BASE_NS::string& a, const BASE_NS::string& b) {
        const auto da = GroupDepth(a);
        const auto db = GroupDepth(b);
        return da != db ? da < db : a < b;
    });
}
}  // namespace

BASE_NS::vector<BASE_NS::string> AliasRegistry::GetContributingGroups(const AliasScopePtr& scope) const
{
    BASE_NS::vector<BASE_NS::string> groups;
    {
        std::shared_lock guard{mutex_};
        auto st = FindScope(scope);
        if (!st) {
            return groups;
        }
        for (auto& e : st->layers) {
            for (auto& c : e.second) {
                if (c.group.empty()) {
                    continue;  // the ungrouped layer is not a group
                }
                if (std::find(groups.begin(), groups.end(), c.group) == groups.end()) {
                    groups.push_back(c.group);
                }
            }
        }
    }
    SortByDepth(groups);
    return groups;
}

BASE_NS::vector<BASE_NS::string> AliasRegistry::GetSettingsGroups(const AliasScopePtr& scope) const
{
    BASE_NS::vector<BASE_NS::string> groups;
    {
        std::shared_lock guard{mutex_};
        auto st = FindScope(scope);
        if (!st) {
            return groups;
        }
        groups.reserve(st->groupSettings.size());
        for (auto& e : st->groupSettings) {
            groups.push_back(e.first);
        }
    }
    SortByDepth(groups);
    return groups;
}

bool AliasRegistry::GroupContribute(BASE_NS::string_view group, BASE_NS::string_view name, const META_NS::IAny* value,
    const TransitionSpec& transition, const AliasScopePtr& scope)
{
    META_NS::IAny::Ptr stored;
    if (value) {
        // A contribution carrying a value establishes the alias, exactly as an ungrouped Define does.
        // A mask does not: it says only that this layer supplies nothing, which needs no type.
        if (!AcquireCell(name, *value, scope)) {
            return false;
        }
        stored = value->Clone(true);
        if (!stored) {
            return false;
        }
    }
    {
        std::unique_lock guard{mutex_};
        RecordContribution(
            AcquireScope(scope), name, Contribution{BASE_NS::string(group), BASE_NS::move(stored), transition});
    }
    return Resolve(name, transition, scope);
}

bool AliasRegistry::GroupUndefine(BASE_NS::string_view group, BASE_NS::string_view name, const AliasScopePtr& scope)
{
    TransitionSpec transition;
    {
        std::unique_lock guard{mutex_};
        auto st = FindScope(scope);
        if (!st || !DropContribution(*st, name, group, transition)) {
            return false;
        }
    }
    return Resolve(name, transition, scope);
}

void AliasRegistry::GroupSetTransition(
    BASE_NS::string_view group, const TransitionSpec& transition, const AliasScopePtr& scope)
{
    std::unique_lock guard{mutex_};
    AcquireScope(scope).groupSettings[group] = transition;
}

TransitionSpec AliasRegistry::GroupTransition(BASE_NS::string_view group, const AliasScopePtr& scope) const
{
    std::shared_lock guard{mutex_};
    auto st = FindScope(scope);
    if (!st) {
        return {};
    }
    // Nearest setting up the path, so that stating one on "theme" governs "theme/accent" as well and a
    // set of interchangeable themes declares once how switching between them behaves. The ungrouped
    // layer's key is empty and has nothing above it, so this reduces to a single lookup for a Define.
    for (auto path = BASE_NS::string(group);;) {
        if (auto it = st->groupSettings.find(path); it != st->groupSettings.end()) {
            return it->second;
        }
        const auto slash = path.rfind('/');
        if (slash == BASE_NS::string::npos) {
            return {};
        }
        path.resize(slash);
    }
}

bool AliasRegistry::GroupSet(
    BASE_NS::string_view group, BASE_NS::array_view<const AliasEntry> values, const AliasScopePtr& scope)
{
    // What the layer decides now, so that whatever it stops deciding can be released afterwards. This is
    // what makes applying one value set over another leave nothing of the first behind.
    auto stale = GroupNames(group, scope);
    bool applied = true;
    for (auto& entry : values) {
        applied = GroupContribute(group, entry.name, entry.value.get(), entry.transition, scope) && applied;
        if (auto it = std::find(stale.begin(), stale.end(), entry.name); it != stale.end()) {
            stale.erase(it);
        }
    }
    for (auto& name : stale) {
        GroupUndefine(group, name, scope);
    }
    return applied;
}

BASE_NS::vector<BASE_NS::string> AliasRegistry::GroupNames(BASE_NS::string_view group, const AliasScopePtr& scope) const
{
    std::shared_lock guard{mutex_};
    BASE_NS::vector<BASE_NS::string> names;
    auto st = FindScope(scope);
    if (!st) {
        return names;
    }
    for (auto& e : st->layers) {
        for (auto& c : e.second) {
            if (c.group == group) {
                names.push_back(e.first);
                break;
            }
        }
    }
    return names;
}

META_END_NAMESPACE()
