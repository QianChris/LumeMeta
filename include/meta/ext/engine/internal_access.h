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

#ifndef META_EXT_ENGINE_INTERNAL_ACCESS_H
#define META_EXT_ENGINE_INTERNAL_ACCESS_H

#include <core/property/scoped_handle.h>

#include <meta/ext/engine/core_any.h>
#include <meta/ext/engine/core_enum_any.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/engine/intf_engine_value.h>
#include <meta/interface/interface_helpers.h>

META_BEGIN_NAMESPACE()

namespace Internal {
template<typename T>
struct IsArray {
    static constexpr const bool VALUE = false;
    using ItemType = T;
};

template<typename T>
struct IsArray<BASE_NS::vector<T>> {
    static constexpr const bool VALUE = true;
    using ItemType = T;
};

template<typename Type, typename... CompatType>
IAny::Ptr ConstructCoreAny(const CORE_NS::Property& p)
{
    if constexpr (Internal::IsArray<Type>::VALUE) {
        using ItemType = typename Internal::IsArray<Type>::ItemType;
        if constexpr ((is_enum_v<Type> || ... || is_enum_v<CompatType>)) {
            return IAny::Ptr(new ArrayCoreEnumAny<ItemType, StaticCastConv, CompatType...>(p));
        }
        return IAny::Ptr(new ArrayCoreAny<ItemType, StaticCastConv, CompatType...>(p));
    } else {
        if constexpr ((is_enum_v<Type> || ... || is_enum_v<CompatType>)) {
            return IAny::Ptr(new CoreEnumAny<Type, StaticCastConv, CompatType...>(p));
        }
        return IAny::Ptr(new CoreAny<Type, StaticCastConv, CompatType...>(p));
    }
}

/**
 * @brief Per-type identity descriptor for the generic scalar engine-value access. Only DATA differs per
 *        engine type (uid/size + a few constructor/compat thunks); the vtable + sync method bodies are
 *        shared (see EngineValueAccessImpl), so they fold to a single copy instead of one
 *        EngineInternalValueAccess<Type> instantiation (vtable + sync code) per engine type.
 */
struct EngineValueAccessInfo {
    BASE_NS::Uid typeUid;  // UidFromType<Type>() — the engine (core) type
    size_t typeSize;       // sizeof(Type)
    IAny::Ptr (*createAny)(const CORE_NS::Property&);
    IAny::Ptr (*createSerializable)();
    IAny::Ptr (*createEngineTemp)();  // SyncTempType (Any<Type>/ArrayAny<ItemType>) for the SyncToEngine probe
    bool (*isCompatible)(const CORE_NS::PropertyTypeDecl&);
};

/**
 * @brief One shared implementation for all scalar engine-value access objects, parameterised only via a
 *        per-INSTANCE EngineValueAccessInfo. The engine read/write is already type-independent: the lock
 *        (IPropertyHandle::R/WLock) returns void*, CalcLocation is uintptr arithmetic, and the value copy
 *        goes through IAny::Get/SetData(uid, ptr, size) which does the typed copy inside the Any. So the
 *        only per-type inputs are {uid, size} + the any-constructor thunks in the descriptor.
 */
class EngineValueAccessImpl : public IntroduceInterfaces<IEngineInternalValueAccess> {
public:
    explicit EngineValueAccessImpl(const EngineValueAccessInfo& info) : info_(info)
    {}

    IAny::Ptr CreateAny(const CORE_NS::Property& p) const override
    {
        return info_.createAny(p);
    }
    IAny::Ptr CreateSerializableAny() const override
    {
        return info_.createSerializable();
    }
    bool IsCompatible(const CORE_NS::PropertyTypeDecl& type) const override
    {
        return info_.isCompatible(type);
    }

    // Byte address of the value inside the locked engine property (0 = out of range).
    static uintptr_t CalcLocation(uintptr_t base, const EnginePropertyParams& params)
    {
        if (auto* cont = params.containerMethods) {
            uintptr_t arrOffset = base + params.arraySubsOffset;
            if (params.index >= cont->size(arrOffset)) {
                return 0;
            }
            return cont->get(arrOffset, params.index) + params.Offset();
        }
        return base + params.Offset();
    }

    AnyReturnValue SyncToEngine(const IAny& value, const EnginePropertyParams& params) const override
    {
        // Probe: value must convert to the engine type and the property must be readable. Only write on an
        // exact SUCCESS; otherwise return the probe result verbatim (NOTHING_TO_DO must NOT become FAIL,
        // callers treat it as "already in sync").
        IAny::Ptr temp = info_.createEngineTemp();
        AnyReturnValue res = (temp && temp->CopyFrom(value)) ? SyncFromEngine(params, *temp) : AnyReturn::FAIL;
        if (res != AnyReturn::SUCCESS) {
            return res;
        }
        CORE_NS::ScopedHandle<uint8_t> guard{params.handle.Handle()};
        uintptr_t loc = guard ? CalcLocation(reinterpret_cast<uintptr_t>(&*guard), params) : 0;
        return loc ? value.GetData(info_.typeUid, reinterpret_cast<void*>(loc), info_.typeSize) : AnyReturn::FAIL;
    }
    AnyReturnValue SyncFromEngine(const EnginePropertyParams& params, IAny& out) const override
    {
        CORE_NS::ScopedHandle<const uint8_t> guard{params.handle.Handle()};
        uintptr_t loc = guard ? CalcLocation(reinterpret_cast<uintptr_t>(&*guard), params) : 0;
        return loc ? out.SetData(info_.typeUid, reinterpret_cast<const void*>(loc), info_.typeSize) : AnyReturn::FAIL;
    }

private:
    const EngineValueAccessInfo& info_;
};

/**
 * @brief Per-type descriptor for the generic array engine-value access (see EngineValueAccessInfo). Note it
 *        does NOT store the array — the Any does; this only marshals engine memory <-> the Any element by
 *        element via the Any's own typed Get/SetDataAt, so there is no per-type element-copy code and no
 *        risky byte-buffer/memcpy (non-trivially-copyable elements like string/EntityReference are handled
 *        correctly by the Any's typed element copy).
 */
struct ArrayEngineValueAccessInfo {
    BASE_NS::Uid itemUid;  // UidFromType<Type>() — the array ELEMENT (core) type
    size_t itemSize;       // sizeof(Type)
    IAny::Ptr (*createAny)(const CORE_NS::Property&);
    IAny::Ptr (*createSerializable)();
    IAny::Ptr (*createArrayTemp)();  // ArrayAny<Type> temp for the SyncToEngine round-trip
    IAny::Ptr (*createItemAny)();    // Any<Type> scalar item, used to fill the target array on read
    bool (*isCompatible)(const CORE_NS::PropertyTypeDecl&);
};

/// One shared implementation for all array engine-value access objects (see EngineValueAccessImpl).
class ArrayEngineValueAccessImpl : public IntroduceInterfaces<IEngineInternalValueAccess> {
public:
    explicit ArrayEngineValueAccessImpl(const ArrayEngineValueAccessInfo& info) : info_(info)
    {}

    IAny::Ptr CreateAny(const CORE_NS::Property& p) const override
    {
        return info_.createAny(p);
    }
    IAny::Ptr CreateSerializableAny() const override
    {
        return info_.createSerializable();
    }
    bool IsCompatible(const CORE_NS::PropertyTypeDecl& type) const override
    {
        return info_.isCompatible(type);
    }
    AnyReturnValue SyncFromEngine(const EnginePropertyParams& params, IAny& out) const override
    {
        auto* arr = interface_cast<IArrayAny>(&out);
        auto* cont = params.property.metaData.containerMethods;
        if (!arr || !cont) {
            return AnyReturn::FAIL;
        }
        CORE_NS::ScopedHandle<const uint8_t> guard{params.handle.Handle()};
        IAny::Ptr item = guard ? info_.createItemAny() : nullptr;
        if (!item) {
            return AnyReturn::FAIL;
        }
        uintptr_t base = reinterpret_cast<uintptr_t>(&*guard);
        size_t n = params.property.type.isArray ? params.property.count : cont->size(base + params.Offset());
        arr->RemoveAll();
        for (size_t i = 0; i != n; ++i) {
            item->SetData(info_.itemUid, reinterpret_cast<const void*>(ElementAddr(base, params, i)), info_.itemSize);
            arr->InsertAnyAt(i, *item);
        }
        return AnyReturn::SUCCESS;
    }
    AnyReturnValue SyncToEngine(const IAny& value, const EnginePropertyParams& params) const override
    {
        IAny::Ptr tempAny = info_.createArrayTemp();
        auto* temp = interface_cast<IArrayAny>(tempAny.get());
        if (!temp || SyncFromEngine(params, *tempAny) != AnyReturn::SUCCESS) {
            return AnyReturn::FAIL;
        }
        AnyReturnValue res = tempAny->CopyFrom(value);
        if (res == AnyReturn::SUCCESS) {
            auto* cont = params.property.metaData.containerMethods;
            CORE_NS::ScopedHandle<uint8_t> guard{params.handle.Handle()};
            if (guard && cont) {
                uintptr_t base = reinterpret_cast<uintptr_t>(&*guard);
                size_t n = temp->GetSize();
                if (params.property.type.isArray) {
                    n = params.property.count < n ? params.property.count : n;
                } else {
                    cont->resize(base + params.Offset(), n);
                }
                for (size_t i = 0; i != n; ++i) {
                    temp->GetDataAt(
                        i, info_.itemUid, reinterpret_cast<void*>(ElementAddr(base, params, i)), info_.itemSize);
                }
            }
        }
        return res;
    }

private:
    // Byte address of array element i inside the locked engine property.
    uintptr_t ElementAddr(uintptr_t base, const EnginePropertyParams& params, size_t i) const
    {
        if (params.property.type.isArray) {
            return base + params.Offset() + i * info_.itemSize;
        }
        return params.property.metaData.containerMethods->get(base + params.Offset(), i);
    }

    const ArrayEngineValueAccessInfo& info_;
};
}  // namespace Internal

/// Class that encapsulates the reading and writing to the core property and the any type used.
/// Thin per-type facade so EngineInternalValueAccess<Type> stays a concrete constructible type (external
/// code registers custom engine types via it). Adds no virtuals, so its vtable is identical to
/// Internal::EngineValueAccessImpl's and folds to one copy; the only per-type residue is the static
/// descriptor + thunks.
template<typename Type, typename AccessType = Type>
class EngineInternalValueAccess : public Internal::EngineValueAccessImpl {
public:
    EngineInternalValueAccess() : Internal::EngineValueAccessImpl(INFO)
    {}

    using SyncTempType = BASE_NS::conditional_t<Internal::IsArray<Type>::VALUE,
        ArrayAny<typename Internal::IsArray<Type>::ItemType>, Any<Type>>;

private:
    static IAny::Ptr CreateAnyThunk(const CORE_NS::Property& p)
    {
        if constexpr (BASE_NS::is_same_v<Type, AccessType>) {
            return Internal::ConstructCoreAny<AccessType>(p);
        } else {
            return Internal::ConstructCoreAny<AccessType, Type>(p);
        }
    }
    static IAny::Ptr CreateSerializableThunk()
    {
        if constexpr (Internal::IsArray<AccessType>::VALUE) {
            using ItemType = typename Internal::IsArray<AccessType>::ItemType;
            return IAny::Ptr(new ArrayAnyType<ItemType>{});
        } else {
            return IAny::Ptr(new AnyType<AccessType>{});
        }
    }
    static IAny::Ptr CreateEngineTempThunk()
    {
        return IAny::Ptr(new SyncTempType{});
    }
    static bool IsCompatibleThunk(const CORE_NS::PropertyTypeDecl& type)
    {
        return MetaType<Type>::coreType == type;
    }

    static constexpr Internal::EngineValueAccessInfo INFO{UidFromType<Type>(),
        sizeof(Type),
        &CreateAnyThunk,
        &CreateSerializableThunk,
        &CreateEngineTempThunk,
        &IsCompatibleThunk};
};

/// Class that encapsulates the reading and writing to the core array property and the any type used.
/// Thin per-type facade (see EngineInternalValueAccess for the scalar rationale).
template<typename Type>
class EngineInternalArrayValueAccess : public Internal::ArrayEngineValueAccessImpl {
public:
    using InternalType = BASE_NS::vector<Type>;
    EngineInternalArrayValueAccess() : Internal::ArrayEngineValueAccessImpl(INFO)
    {}

private:
    static IAny::Ptr CreateAnyThunk(const CORE_NS::Property& p)
    {
        return Internal::ConstructCoreAny<InternalType>(p);
    }
    static IAny::Ptr CreateSerializableThunk()
    {
        return IAny::Ptr(new ArrayAnyType<Type>{});
    }
    static IAny::Ptr CreateArrayTempThunk()
    {
        return IAny::Ptr(new ArrayAny<Type>{});
    }
    static IAny::Ptr CreateItemThunk()
    {
        return IAny::Ptr(new Any<Type>{});
    }
    static bool IsCompatibleThunk(const CORE_NS::PropertyTypeDecl& type)
    {
        return MetaType<Type[]>::coreType == type;
    }

    static constexpr Internal::ArrayEngineValueAccessInfo INFO{UidFromType<Type>(),
        sizeof(Type),
        &CreateAnyThunk,
        &CreateSerializableThunk,
        &CreateArrayTempThunk,
        &CreateItemThunk,
        &IsCompatibleThunk};
};

META_END_NAMESPACE()

#endif
