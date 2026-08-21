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
#ifndef META_INTERFACE_DETAIL_INTERFACE_ANY_H
#define META_INTERFACE_DETAIL_INTERFACE_ANY_H

#include <meta/interface/detail/base_any.h>

META_BEGIN_NAMESPACE()

// Aliasing-cast an interface shared_ptr to the interface identified by uid: shares the refcount but
// points at the correctly-offset subobject obtained via the virtual GetInterface. This is NOT a
// reinterpret_cast: interface subobject addresses differ per interface, so the raw pointer must be
// round-tripped through GetInterface. Reinterpreting the shared_ptr *layout* (to read the control block
// or call the virtual GetInterface) is valid; assuming the element pointers coincide is not.
template<typename IIPtr>
inline IIPtr InterfaceAliasCast(const IIPtr& sp, const BASE_NS::Uid& uid)
{
    if (sp) {
        if (auto* p = sp->GetInterface(uid)) {
            return IIPtr(sp, static_cast<typename IIPtr::element_type*>(p));
        }
    }
    return {};
}

/**
 * @brief Compile-time compatibility policy for the generic interface-pointer Anys, parameterised only by
 *        const-ness/weak-ness. Handles the type-erased compat forms (shared_ptr<[const] IInterface> /
 *        weak_ptr<...>): the compatible-type lists per direction and the get/set/validate conversions.
 *        Sole consumer is InterfacePtrAnyImpl / ArrayInterfacePtrAnyImpl below; the widely-used aliases
 *        (SharedPtrIInterface etc.) still live in any_pointer_compatibility.h.
 */
template<bool IsConst, bool IsWeak>
struct AnyPointerCompatibility {
    using IIType = BASE_NS::conditional_t<IsConst, const CORE_NS::IInterface, CORE_NS::IInterface>;
    using IIPtrType = BASE_NS::conditional_t<IsConst, SharedPtrConstIInterface, SharedPtrIInterface>;

    template<typename Type>
    static BASE_NS::array_view<const TypeId> GetCompatibleTypes(CompatibilityDirection dir)
    {
        static constexpr TypeId typeId = UidFromType<Type>();
        if (dir == CompatibilityDirection::GET) {
            if constexpr (IsConst) {
                static TypeId uids[] = {typeId, SharedPtrConstIInterfaceId};
                return uids;
            }
            static TypeId uids[] = {typeId, SharedPtrIInterfaceId, SharedPtrConstIInterfaceId};
            return uids;
        }
        if (dir == CompatibilityDirection::SET) {
            if constexpr (IsConst) {
                static TypeId uids[] = {typeId, SharedPtrIInterfaceId, SharedPtrConstIInterfaceId};
                return uids;
            }
            static TypeId uids[] = {typeId, SharedPtrIInterfaceId};
            return uids;
        }
        if constexpr (IsConst) {
            static TypeId uids[] = {typeId, SharedPtrConstIInterfaceId};
            return uids;
        }

        static TypeId uids[] = {typeId, SharedPtrIInterfaceId};
        return uids;
    }

    static AnyReturnValue GetData(const TypeId& id, void* data, const IIPtrType& ptr)
    {
        if constexpr (!IsConst) {
            if (id == SharedPtrIInterfaceId) {
                *static_cast<SharedPtrIInterface*>(data) = ptr;
                return AnyReturn::SUCCESS;
            }
        }
        if (id == SharedPtrConstIInterfaceId) {
            *static_cast<SharedPtrConstIInterface*>(data) = ptr;
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INVALID_ARGUMENT;
    }

    static bool SetData(const TypeId& id, const void* data, IIPtrType& out)
    {
        if constexpr (IsConst) {
            if (id == SharedPtrConstIInterfaceId) {
                out = *static_cast<const SharedPtrConstIInterface*>(data);
                return true;
            }
        }
        if (id == SharedPtrIInterfaceId) {
            out = *static_cast<const SharedPtrIInterface*>(data);
            return true;
        }
        return false;
    }
    static constexpr bool IsValidGetArgs(const TypeId& uid, const void* data, size_t size)
    {
        if constexpr (!IsConst) {
            if (data && uid == SharedPtrIInterfaceId && sizeof(SharedPtrIInterface) == size) {
                return true;
            }
        }
        if (data && uid == SharedPtrConstIInterfaceId && sizeof(SharedPtrConstIInterface) == size) {
            return true;
        }
        return false;
    }
    static constexpr bool IsValidSetArgs(const TypeId& uid, const void* data, size_t size)
    {
        if constexpr (IsConst) {
            if (data && uid == SharedPtrConstIInterfaceId && sizeof(SharedPtrConstIInterface) == size) {
                return true;
            }
        }
        if (data && uid == SharedPtrIInterfaceId && sizeof(SharedPtrIInterface) == size) {
            return true;
        }
        return false;
    }
};

template<bool IsConst>
struct AnyPointerCompatibility<IsConst, true> {
    using IIType = BASE_NS::conditional_t<IsConst, const CORE_NS::IInterface, CORE_NS::IInterface>;
    using IIPtrType = BASE_NS::conditional_t<IsConst, SharedPtrConstIInterface, SharedPtrIInterface>;

    template<typename Type>
    static BASE_NS::array_view<const TypeId> GetCompatibleTypes(CompatibilityDirection dir)
    {
        static constexpr TypeId typeId = UidFromType<Type>();
        if (dir == CompatibilityDirection::GET) {
            if constexpr (IsConst) {
                static TypeId uids[] = {typeId, SharedPtrConstIInterfaceId, WeakPtrConstIInterfaceId};
                return uids;
            }
            static TypeId uids[] = {typeId,
                SharedPtrIInterfaceId,
                SharedPtrConstIInterfaceId,
                WeakPtrIInterfaceId,
                WeakPtrConstIInterfaceId};
            return uids;
        }
        if (dir == CompatibilityDirection::SET) {
            if constexpr (IsConst) {
                static TypeId uids[] = {typeId,
                    SharedPtrIInterfaceId,
                    SharedPtrConstIInterfaceId,
                    WeakPtrIInterfaceId,
                    WeakPtrConstIInterfaceId};
                return uids;
            }
            static TypeId uids[] = {typeId, SharedPtrIInterfaceId, WeakPtrIInterfaceId};
            return uids;
        }
        if constexpr (IsConst) {
            static TypeId uids[] = {typeId, SharedPtrConstIInterfaceId, WeakPtrConstIInterfaceId};
            return uids;
        }

        static TypeId uids[] = {typeId, SharedPtrIInterfaceId, WeakPtrIInterfaceId};
        return uids;
    }

    static AnyReturnValue GetData(const TypeId& id, void* data, const IIPtrType& ptr)
    {
        if (AnyPointerCompatibility<IsConst, false>::GetData(id, data, ptr)) {
            return AnyReturn::SUCCESS;
        }
        if constexpr (!IsConst) {
            if (id == WeakPtrIInterfaceId) {
                *static_cast<WeakPtrIInterface*>(data) = ptr;
                return AnyReturn::SUCCESS;
            }
        }
        if (id == WeakPtrConstIInterfaceId) {
            *static_cast<WeakPtrConstIInterface*>(data) = ptr;
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INVALID_ARGUMENT;
    }

    static bool SetData(const TypeId& id, const void* data, IIPtrType& out)
    {
        if (AnyPointerCompatibility<IsConst, false>::SetData(id, data, out)) {
            return true;
        }
        if constexpr (IsConst) {
            if (id == WeakPtrConstIInterfaceId) {
                out = static_cast<const WeakPtrConstIInterface*>(data)->lock();
                return true;
            }
        }
        if (id == WeakPtrIInterfaceId) {
            out = static_cast<const WeakPtrIInterface*>(data)->lock();
            return true;
        }
        return false;
    }
    static constexpr bool IsValidGetArgs(const TypeId& uid, const void* data, size_t size)
    {
        if (AnyPointerCompatibility<IsConst, false>::IsValidGetArgs(uid, data, size)) {
            return true;
        }
        if constexpr (!IsConst) {
            if (data && uid == WeakPtrIInterfaceId && sizeof(WeakPtrIInterface) == size) {
                return true;
            }
        }
        if (data && uid == WeakPtrConstIInterfaceId && sizeof(WeakPtrConstIInterface) == size) {
            return true;
        }
        return false;
    }
    static constexpr bool IsValidSetArgs(const TypeId& uid, const void* data, size_t size)
    {
        if (AnyPointerCompatibility<IsConst, false>::IsValidSetArgs(uid, data, size)) {
            return true;
        }
        if constexpr (IsConst) {
            if (data && uid == WeakPtrConstIInterfaceId && sizeof(WeakPtrConstIInterface) == size) {
                return true;
            }
        }
        if (data && uid == WeakPtrIInterfaceId && sizeof(WeakPtrIInterface) == size) {
            return true;
        }
        return false;
    }
};

/**
 * @brief Per-interface identity descriptor for the generic interface-pointer scalar Anys. Only DATA
 *        differs per interface type; the code/vtable is shared (see InterfacePtrAnyImpl).
 */
struct InterfacePtrAnyInfo {
    AnyTypeInfo any;
    ObjectId arrayClassId;      // for the scalar->array Clone
    BASE_NS::Uid interfaceUid;  // the IXxx interface UID, used to reconstruct the typed pointer
};

/**
 * @brief One shared implementation for all interface-pointer scalar Anys (IXxx::Ptr / ::ConstPtr /
 *        ::WeakPtr / ::ConstWeakPtr), parameterised only by const-ness/weak-ness (4 instantiations), NOT
 *        by the interface type. The interface identity is carried per-INSTANCE via an InterfacePtrAnyInfo
 *        pointer, so the vtable + method bodies fold to a single copy instead of a distinct
 *        Any<IXxx::Ptr> specialisation (vtable + descriptor) per interface. Storage is the type-erased
 *        shared/weak_ptr<[const] IInterface>.
 */
template<bool IsConst, bool IsWeak>
class InterfacePtrAnyImpl : public AnyBase {
public:
    using PC = AnyPointerCompatibility<IsConst, IsWeak>;
    using IIType = typename PC::IIType;        // [const] IInterface
    using IIPtrType = typename PC::IIPtrType;  // shared_ptr<[const] IInterface>
    using StoredType = BASE_NS::conditional_t<IsWeak, BASE_NS::weak_ptr<IIType>, IIPtrType>;

    InterfacePtrAnyImpl(const InterfacePtrAnyInfo* info, StoredType value) : info_(info), value_(BASE_NS::move(value))
    {}

    static IIPtrType Lock(const StoredType& v)
    {
        if constexpr (IsWeak) {
            return v.lock();
        } else {
            return v;
        }
    }
    // Read the concrete IXxx::(Weak)Ptr in `data` as an interface shared_ptr (layout-compatible read).
    static IIPtrType LockData(const void* data)
    {
        if constexpr (IsWeak) {
            return reinterpret_cast<const BASE_NS::weak_ptr<IIType>*>(data)->lock();
        } else {
            return *reinterpret_cast<const IIPtrType*>(data);
        }
    }

protected:
    const AnyTypeInfo& Info() const override
    {
        return info_->any;
    }
    void Store(const IIPtrType& p)
    {
        value_ = p;
    }
    AnyReturnValue RawGet(void* data) const override
    {
        *static_cast<StoredType*>(data) = InterfaceAliasCast(Lock(value_), info_->interfaceUid);
        return AnyReturn::SUCCESS;
    }
    AnyReturnValue RawSet(const void* data) override
    {
        IIPtrType canonical = InterfaceAliasCast(LockData(data), CORE_NS::IInterface::UID);
        if constexpr (!IsWeak) {
            if (value_ == canonical) {
                return AnyReturn::NOTHING_TO_DO;
            }
        }
        value_ = canonical;
        return AnyReturn::SUCCESS;
    }
    AnyReturnValue RawGetCompat(const TypeId& id, void* data, size_t size) const override
    {
        if (data && PC::IsValidGetArgs(id, data, size)) {
            return PC::GetData(id, data, Lock(value_));
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue RawSetCompat(const TypeId& id, const void* data, size_t size) override
    {
        if (data && PC::IsValidSetArgs(id, data, size)) {
            IIPtrType p;
            if (PC::SetData(id, data, p)) {
                Store(p);
                return AnyReturn::SUCCESS;
            }
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue RawCopyFrom(const IAny& any) override
    {
        IIPtrType p;
        if (any.GetValue(p)) {
            Store(p);
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::FAIL;
    }
    AnyReturnValue RawReset() override
    {
        value_ = {};
        return AnyReturn::SUCCESS;
    }
    IAny::Ptr RawClone(const AnyCloneOptions& options) const override
    {
        if (options.role == TypeIdRole::ARRAY) {
            return ConstructRegisteredAny(info_->arrayClassId);
        }
        return IAny::Ptr(
            new InterfacePtrAnyImpl(info_, options.value == CloneValueType::COPY_VALUE ? value_ : StoredType{}));
    }

    const InterfacePtrAnyInfo* info_;
    StoredType value_;
};

/**
 * @brief Thin per-interface facade so AnyType<IXxx::Ptr> resolves to a concrete constructible type and so
 *        registration keeps the per-type class id / name. Adds no virtuals, so its vtable is identical to
 *        InterfacePtrAnyImpl's and folds to a single copy; the only per-type residue is the static
 *        descriptor (data) + the compatible-types accessor.
 */
template<typename Type>
class InterfacePtrAny : public InterfacePtrAnyImpl<IsConstPtr_v<Type>, IsWeakPtr_v<Type>> {
    using Super = InterfacePtrAnyImpl<IsConstPtr_v<Type>, IsWeakPtr_v<Type>>;

public:
    using StoredType = typename Super::StoredType;
    using ElementType = BASE_NS::remove_const_t<typename Type::element_type>;
    static constexpr TypeId TYPE_ID = UidFromType<Type>();

    static constexpr ObjectId StaticGetClassId()
    {
        return MakeUid<Type>(BUILTIN_ANY_TAG);
    }
    static constexpr BASE_NS::string_view StaticGetTypeIdString()
    {
        return MetaType<Type>::name;
    }
    static const BASE_NS::array_view<const TypeId> StaticGetCompatibleTypes(CompatibilityDirection dir)
    {
        return Super::PC::template GetCompatibleTypes<Type>(dir);
    }

    // The shared per-interface descriptor; also referenced by the array facade's item-clone.
    static constexpr InterfacePtrAnyInfo INFO{
        AnyTypeInfo{
            StaticGetClassId(),
            TYPE_ID,
            ArrayUidFromType<Type>(),
            ItemUidFromType<Type>(),
            StaticGetTypeIdString(),
            sizeof(Type),
            &StaticGetCompatibleTypes,
        },
        MakeUid<Type>(BUILTIN_ARRAY_ANY_TAG),
        ElementType::UID,
    };

    InterfacePtrAny(Type v = {}) : Super(&INFO, ToCanonical(v))
    {}

    // Concrete-typed read-back for generic callers using the Any<T> fast-path. Returns by value (the
    // value is stored type-erased and reconstructed on demand); a caller binding it to `const Type&`
    // keeps it alive via temporary lifetime extension.
    Type InternalGetValue() const
    {
        Type v{};
        this->GetData(TYPE_ID, &v, sizeof(Type));
        return v;
    }
    // Keep IAny::GetTypeId(TypeIdRole) visible: the no-arg fast path declared here would
    // otherwise hide the virtual overload (-Woverloaded-virtual).
    using Super::GetTypeId;
    TypeId GetTypeId() const
    {
        return TYPE_ID;
    }

private:
    static StoredType ToCanonical(const Type& v)
    {
        if constexpr (IsWeakPtr_v<Type>) {
            return interface_pointer_cast<typename Super::IIType>(v.lock());
        } else {
            return interface_pointer_cast<typename Super::IIType>(v);
        }
    }
};

/**
 * @brief Per-interface identity descriptor for the generic interface-pointer array Anys.
 */
struct InterfaceArrayAnyInfo {
    ArrayAnyTypeInfo array;
    BASE_NS::Uid interfaceUid;
    const InterfacePtrAnyInfo* scalar;  // for the array->item Clone (constructs the generic scalar)
};

/**
 * @brief One shared implementation for all interface-pointer array Anys, mirroring InterfacePtrAnyImpl
 *        over a vector of type-erased interface pointers. Erasing the array in lockstep with the scalar is
 *        what makes the collapse net-positive: the array->item Clone builds the generic scalar directly
 *        (no per-type Any<IXxx::Ptr>, no registry lookup), breaking the Any/ArrayAny coupling.
 */
template<bool IsConst, bool IsWeak>
class ArrayInterfacePtrAnyImpl : public ArrayAnyBase {
public:
    using PC = AnyPointerCompatibility<IsConst, IsWeak>;
    using IIType = typename PC::IIType;
    using IIPtrType = typename PC::IIPtrType;
    using StoredItem = BASE_NS::conditional_t<IsWeak, BASE_NS::weak_ptr<IIType>, IIPtrType>;
    using ArrayType = BASE_NS::vector<StoredItem>;

    ArrayInterfacePtrAnyImpl(const InterfaceArrayAnyInfo* info, ArrayType value)
        : info_(info), value_(BASE_NS::move(value))
    {}

    static IIPtrType Lock(const StoredItem& v)
    {
        if constexpr (IsWeak) {
            return v.lock();
        } else {
            return v;
        }
    }
    // Canonicalise a concrete item slot (IXxx::(Weak)Ptr, layout-compatible with StoredItem) to IInterface.
    static IIPtrType CanonicalFromItem(const void* item)
    {
        IIPtrType p;
        if constexpr (IsWeak) {
            p = reinterpret_cast<const BASE_NS::weak_ptr<IIType>*>(item)->lock();
        } else {
            p = *reinterpret_cast<const IIPtrType*>(item);
        }
        return InterfaceAliasCast(p, CORE_NS::IInterface::UID);
    }

    AnyReturnValue SetAnyAt(IArrayAny::IndexType index, const IAny& value) override
    {
        StoredItem v{};
        if (value.GetData(info_->array.itemId, &v, info_->array.itemSize)) {
            return SetDataAt(index, info_->array.itemId, &v, info_->array.itemSize);
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue GetAnyAt(IArrayAny::IndexType index, IAny& value) const override
    {
        StoredItem v{};
        if (GetDataAt(index, info_->array.itemId, &v, info_->array.itemSize)) {
            return value.SetData(info_->array.itemId, &v, info_->array.itemSize);
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue InsertAnyAt(IArrayAny::IndexType index, const IAny& value) override
    {
        StoredItem v{};
        if (value.GetData(info_->array.itemId, &v, info_->array.itemSize)) {
            index = index < value_.size() ? index : value_.size();
            value_.insert(value_.begin() + index, StoredItem{CanonicalFromItem(&v)});
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INVALID_ARGUMENT;
    }

protected:
    const ArrayAnyTypeInfo& Info() const override
    {
        return info_->array;
    }
    // Build a concrete-layout vector (each element aliased to the interface type) for the caller.
    ArrayType ToConcrete() const
    {
        ArrayType out;
        out.reserve(value_.size());
        for (const auto& e : value_) {
            out.push_back(StoredItem{InterfaceAliasCast(Lock(e), info_->interfaceUid)});
        }
        return out;
    }
    void RawGetVector(void* out) const override
    {
        *static_cast<ArrayType*>(out) = ToConcrete();
    }
    AnyReturnValue RawSetVector(const void* in) override
    {
        const auto& concrete = *reinterpret_cast<const ArrayType*>(in);
        ArrayType next;
        next.reserve(concrete.size());
        for (const auto& e : concrete) {
            next.push_back(StoredItem{InterfaceAliasCast(Lock(e), CORE_NS::IInterface::UID)});
        }
        value_ = BASE_NS::move(next);
        return AnyReturn::SUCCESS;
    }
    AnyReturnValue RawSetFromArray(const void* data, size_t count) override
    {
        auto* items = reinterpret_cast<const StoredItem*>(data);
        ArrayType next;
        next.reserve(count);
        for (size_t i = 0; i != count; ++i) {
            next.push_back(StoredItem{InterfaceAliasCast(Lock(items[i]), CORE_NS::IInterface::UID)});
        }
        value_ = BASE_NS::move(next);
        return AnyReturn::SUCCESS;
    }
    void RawData(const void*& ptr, size_t& count) const override
    {
        // Best-effort raw view (interface-canonical pointers); the meaningful path is the vector above.
        ptr = value_.data();
        count = value_.size();
    }
    IArrayAny::IndexType RawSize() const override
    {
        return value_.size();
    }
    AnyReturnValue RawGetItem(size_t index, void* out) const override
    {
        *static_cast<StoredItem*>(out) = InterfaceAliasCast(Lock(value_[index]), info_->interfaceUid);
        return AnyReturn::SUCCESS;
    }
    AnyReturnValue RawSetItem(size_t index, const void* in) override
    {
        value_[index] = CanonicalFromItem(in);
        return AnyReturn::SUCCESS;
    }
    AnyReturnValue RawRemoveAt(size_t index) override
    {
        if (index < value_.size()) {
            value_.erase(value_.begin() + index);
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    void RawRemoveAll() override
    {
        value_.clear();
    }
    AnyReturnValue RawCopyFrom(const IAny& any) override
    {
        ArrayType concrete;
        if (any.GetData(info_->array.vectorId, &concrete, info_->array.vectorSize)) {
            ArrayType next;
            next.reserve(concrete.size());
            for (const auto& e : concrete) {
                next.push_back(StoredItem{InterfaceAliasCast(Lock(e), CORE_NS::IInterface::UID)});
            }
            value_ = BASE_NS::move(next);
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::FAIL;
    }
    AnyReturnValue RawReset() override
    {
        value_.clear();
        return AnyReturn::SUCCESS;
    }
    IAny::Ptr RawClone(const AnyCloneOptions& options) const override
    {
        if (options.role == TypeIdRole::ITEM) {
            return IAny::Ptr(new InterfacePtrAnyImpl<IsConst, IsWeak>(info_->scalar, {}));
        }
        return IAny::Ptr(
            new ArrayInterfacePtrAnyImpl(info_, options.value == CloneValueType::COPY_VALUE ? value_ : ArrayType{}));
    }

    const InterfaceArrayAnyInfo* info_;
    ArrayType value_;
};

/**
 * @brief Thin per-interface facade for the array any (see InterfacePtrAny for the scalar rationale).
 */
template<typename Type>
class InterfacePtrArrayAny : public ArrayInterfacePtrAnyImpl<IsConstPtr_v<Type>, IsWeakPtr_v<Type>> {
    using Super = ArrayInterfacePtrAnyImpl<IsConstPtr_v<Type>, IsWeakPtr_v<Type>>;

public:
    using StoredItem = typename Super::StoredItem;
    using ArrayType = typename Super::ArrayType;
    using ElementType = BASE_NS::remove_const_t<typename Type::element_type>;
    static constexpr TypeId ARRAY_TYPE_ID = ArrayUidFromType<Type>();
    static constexpr TypeId VECTOR_TYPE_ID = UidFromType<BASE_NS::vector<Type>>();

    static constexpr ObjectId StaticGetClassId()
    {
        return MakeUid<Type>(BUILTIN_ARRAY_ANY_TAG);
    }
    static constexpr BASE_NS::string_view StaticGetTypeIdString()
    {
        return MetaType<BASE_NS::vector<Type>>::name;
    }
    static const BASE_NS::array_view<const TypeId> StaticGetCompatibleTypes(CompatibilityDirection)
    {
        static constexpr TypeId uids[] = {ARRAY_TYPE_ID, VECTOR_TYPE_ID};
        return uids;
    }
    static const BASE_NS::array_view<const TypeId> StaticGetItemCompatibleTypes(CompatibilityDirection dir)
    {
        return InterfacePtrAny<Type>::StaticGetCompatibleTypes(dir);
    }

    static constexpr InterfaceArrayAnyInfo INFO{
        ArrayAnyTypeInfo{
            StaticGetClassId(),
            ARRAY_TYPE_ID,
            ItemUidFromType<Type>(),
            VECTOR_TYPE_ID,
            StaticGetTypeIdString(),
            sizeof(BASE_NS::vector<Type>),
            sizeof(Type),
            &StaticGetCompatibleTypes,
            &StaticGetItemCompatibleTypes,
        },
        ElementType::UID,
        &InterfacePtrAny<Type>::INFO,
    };

    InterfacePtrArrayAny(BASE_NS::vector<Type> v = {}) : Super(&INFO, ToCanonical(v))
    {}
    InterfacePtrArrayAny(BASE_NS::array_view<const Type> v) : Super(&INFO, ToCanonical(v))
    {}

    // Concrete-typed read-back for generic callers using the ArrayAny<T> fast-path (see InterfacePtrAny).
    BASE_NS::vector<Type> InternalGetValue() const
    {
        BASE_NS::vector<Type> v;
        this->GetData(VECTOR_TYPE_ID, &v, sizeof(v));
        return v;
    }
    // Keep IAny::GetTypeId(TypeIdRole) visible: the no-arg fast path declared here would
    // otherwise hide the virtual overload (-Woverloaded-virtual).
    using Super::GetTypeId;
    TypeId GetTypeId() const
    {
        return ARRAY_TYPE_ID;
    }

private:
    template<typename Range>
    static ArrayType ToCanonical(const Range& v)
    {
        ArrayType out;
        out.reserve(v.size());
        for (const auto& e : v) {
            if constexpr (IsWeakPtr_v<Type>) {
                out.push_back(interface_pointer_cast<typename Super::IIType>(e.lock()));
            } else {
                out.push_back(interface_pointer_cast<typename Super::IIType>(e));
            }
        }
        return out;
    }
};

// A direct `Any<IFoo::Ptr>` / `ArrayAny<IFoo::Ptr>` in user code resolves to the shared interface-pointer
// implementation — exactly like `AnyType<IFoo::Ptr>` — so no concrete per-type Any/ArrayAny is emitted.
template<typename Type, typename Compare>
class Any<Type, Compare, EnableSpecialisation<IsInterfacePtr_v<Type>>> : public InterfacePtrAny<Type> {
public:
    using InterfacePtrAny<Type>::InterfacePtrAny;
};

template<typename Type, typename Compare>
class ArrayAny<Type, Compare, EnableSpecialisation<IsInterfacePtr_v<Type>>> : public InterfacePtrArrayAny<Type> {
public:
    using InterfacePtrArrayAny<Type>::InterfacePtrArrayAny;
};

META_END_NAMESPACE()

#endif
