/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef META_ENGINE_INTERFACE_ENGINE_VALUE_MANAGER_H
#define META_ENGINE_INTERFACE_ENGINE_VALUE_MANAGER_H

#include <meta/interface/engine/intf_engine_value.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/property/array_property.h>
#include <meta/interface/property/property.h>
#include <meta/interface/property/property_events.h>
#include <meta/interface/simple_event.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IEngineValueManager, "542b7e13-21bd-4c9c-8d3f-a9527732216c")

class IEngineValueManager;

/**
 * @brief Callable invoked when a manager acquires values written but not yet synced to the engine.
 *        Receives the manager itself. @see IEngineValueManager::SetSyncScheduler.
 */
struct IOnEngineSyncNeededInfo {
    constexpr static BASE_NS::Uid UID{"ccf028a8-2970-4e6c-a7de-518c9bd52a3a"};
    constexpr static char const* NAME{"OnEngineSyncNeeded"};
};
using IOnEngineSyncNeeded = SimpleEvent<IOnEngineSyncNeededInfo, void(const BASE_NS::shared_ptr<IEngineValueManager>&)>;

/**
 * @brief Options for constructing engine values
 */
struct EngineValueOptions {
    /// prefix that is added before the name (plus '.')
    BASE_NS::string namePrefix;
    /// optional output vector to put all matching values
    BASE_NS::vector<IEngineValue::Ptr>* values{};
    /**
     * @brief Push value directly to engine when set, if this is enabled,
     *        the values can only be set in the engine task queue thread
     **/
    bool pushValuesDirectlyToEngine{};
    /**
     * @brief If true, also recursively handle known struct types. E.g. Vec4 would be expanded into x, y, z, and w
     * properties.
     */
    bool recurseKnownStructs{};
};

/**
 * @brief Manager that contains engine values, handles their synchronisation and notification of changes
 */
class IEngineValueManager : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEngineValueManager)
public:
    /**
     * @brief Set task queue where the engine value change notifications are posted to.
     *        This must be set to be able to get notifications.
     */
    virtual void SetNotificationQueue(const ITaskQueue::WeakPtr&) = 0;

    /**
     * @brief Called internally by engine values when they become dirty.
     *        Invokes the callable set via SetSyncScheduler.
     */
    virtual void NotifyDirty() = 0;

    /**
     * @brief Synchronise engine values with core engine.
     *        When componentManager is non-null, only values belonging to that manager are synced.
     * @note  This must be called in correct thread because it touches the core engine
     */
    virtual bool Sync(EngineSyncDirection dir, const CORE_NS::IComponentManager* componentManager) = 0;

    /** @brief Convenience overload that syncs all values. */
    bool Sync(EngineSyncDirection dir)
    {
        return Sync(dir, nullptr);
    }

    /// Add engine values from the handle
    virtual bool ConstructValues(EnginePropertyHandle handle, EngineValueOptions) = 0;
    // Add engine values from handle
    virtual bool ConstructValues(CORE_NS::IPropertyHandle* handle, EngineValueOptions) = 0;
    // Add engine values from value (e.g. member properties or follow EnginePropertyHandle)
    virtual bool ConstructValues(IValue::Ptr value, EngineValueOptions) = 0;
    // Add single engine value from EnginePropertyParams
    virtual bool ConstructValue(EnginePropertyParams property, EngineValueOptions) = 0;
    /// Add single engine value starting from handle and following path
    virtual bool ConstructValue(EnginePropertyHandle handle, BASE_NS::string_view path, EngineValueOptions) = 0;

    /// Remove engine value with given name
    virtual bool RemoveValue(BASE_NS::string_view name) = 0;
    /// Remove all engine values
    virtual void RemoveAll() = 0;

    /// Construct property from engine value with given name
    virtual IProperty::Ptr ConstructProperty(BASE_NS::string_view name) const = 0;
    /// Get engine value of given name
    virtual IEngineValue::Ptr GetEngineValue(BASE_NS::string_view name) const = 0;
    /// Construct property from all contained engine values
    virtual BASE_NS::vector<IProperty::Ptr> ConstructAllProperties() const = 0;
    /// Get all engine values
    virtual BASE_NS::vector<IEngineValue::Ptr> GetAllEngineValues() const = 0;
    /// Check if any engine values exist
    virtual bool HasValues() const = 0;

    /**
     * @brief Set a callable invoked when this manager acquires values written but not yet synced to
     *        the engine.
     *
     * Writes are batched, and the batch is settled later by whoever is notified here. The callable
     * is handed the manager rather than left to find it, so that holding on to what it is given is
     * enough to guarantee the batch survives: a value written just before its owner is dropped is
     * still there to be synced. Keeping that reference until the sync has run is the receiver's
     * side of the contract -- dropping it immediately loses the writes, exactly as a bare notifier
     * that scheduled a weakly held owner would.
     *
     * The manager holds the callable, so the callable must not hold the manager.
     */
    virtual void SetSyncScheduler(const IOnEngineSyncNeeded::InterfaceTypePtr& scheduler) = 0;

    /// True when values have been written but not yet synced to the engine.
    virtual bool HasPendingSync() const = 0;
    /// Construct property from engine value with given name
    template<typename Type>
    Property<Type> ConstructProperty(BASE_NS::string_view name)
    {
        return Property<Type>(ConstructProperty(name));
    }
    /// Construct array property from engine value with given name
    template<typename Type>
    ArrayProperty<Type> ConstructArrayProperty(BASE_NS::string_view name)
    {
        return ArrayProperty<Type>(ConstructProperty(name));
    }
    /// Add engine values from the handle with default options
    bool ConstructValues(EnginePropertyHandle handle)
    {
        return ConstructValues(handle, {});
    }
};

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IEngineValueManager)

#endif
