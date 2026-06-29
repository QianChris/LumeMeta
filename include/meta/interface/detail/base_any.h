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
#ifndef META_INTERFACE_DETAIL_BASE_ANY_H
#define META_INTERFACE_DETAIL_BASE_ANY_H

#include <meta/interface/detail/any_pointer_compatibility.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_value.h>

META_BEGIN_NAMESPACE()

constexpr char BUILTIN_ANY_TAG[] = "BuiltAny";
constexpr char BUILTIN_ARRAY_ANY_TAG[] = "ArrayAny";

/**
 * @brief Per-type identity descriptor for the single-value builtin Any implementations.
 *
 * Carries the type's identity (uids, name, size) as data so the type-independent IAny surface can
 * live once in the non-template AnyBase instead of being stamped out per value type.
 */
struct AnyTypeInfo {
    ObjectId classId;
    TypeId typeId;
    TypeId arrayId;
    TypeId itemId;
    BASE_NS::string_view typeName;
    size_t size;
    const BASE_NS::array_view<const TypeId> (*getCompatibleTypes)(CompatibilityDirection);
};

/**
 * @brief Type-erased base shared by all single-value builtin Any types.
 *
 * Implements the entire type-independent IAny/IValue surface a single value Any needs, driven by a
 * per-type AnyTypeInfo descriptor and a handful of small hooks the typed subclass supplies. The
 * heavy method bodies are compiled once here rather than once per value type.
 */
class AnyBase : public IntroduceInterfaces<IAny, IValue> {
public:
    ObjectId GetClassId() const override
    {
        return Info().classId;
    }
    const BASE_NS::array_view<const TypeId> GetCompatibleTypes(CompatibilityDirection dir) const override
    {
        return Info().getCompatibleTypes(dir);
    }
    AnyReturnValue GetData(const TypeId& id, void* data, size_t size) const override
    {
        const auto& info = Info();
        if (data && id == info.typeId && size == info.size) {
            return RawGet(data);
        }
        return RawGetCompat(id, data, size);
    }
    AnyReturnValue SetData(const TypeId& id, const void* data, size_t size) override
    {
        const auto& info = Info();
        if (data && id == info.typeId && size == info.size) {
            return RawSet(data);
        }
        return RawSetCompat(id, data, size);
    }
    AnyReturnValue CopyFrom(const IAny& any) override
    {
        return RawCopyFrom(any);
    }
    AnyReturnValue ResetValue() override
    {
        return RawReset();
    }
    IAny::Ptr Clone(const AnyCloneOptions& options) const override
    {
        return RawClone(options);
    }
    TypeId GetTypeId(TypeIdRole role) const override
    {
        const auto& info = Info();
        if (role == TypeIdRole::ARRAY) {
            return info.arrayId;
        }
        if (role == TypeIdRole::ITEM) {
            return info.itemId;
        }
        return info.typeId;
    }
    BASE_NS::string GetTypeIdString() const override
    {
        return BASE_NS::string(Info().typeName);
    }
    using IAny::SetValue;
    AnyReturnValue SetValue(const IAny& value) override
    {
        return CopyFrom(value);
    }
    using IAny::GetValue;
    const IAny& GetValue() const override
    {
        return *this;
    }
    bool IsCompatible(const TypeId& id) const override
    {
        return META_NS::IsCompatible(*this, id);
    }

protected:
    virtual const AnyTypeInfo& Info() const = 0;
    // Read/write the value when the requested id is the type's own id.
    virtual AnyReturnValue RawGet(void* data) const = 0;
    virtual AnyReturnValue RawSet(const void* data) = 0;
    // Read/write the value for compatible ids (e.g. interface-pointer aliases). Returns INVALID if none.
    virtual AnyReturnValue RawGetCompat(const TypeId& id, void* data, size_t size) const = 0;
    virtual AnyReturnValue RawSetCompat(const TypeId& id, const void* data, size_t size) = 0;
    virtual AnyReturnValue RawCopyFrom(const IAny& any) = 0;
    virtual AnyReturnValue RawReset() = 0;
    virtual IAny::Ptr RawClone(const AnyCloneOptions& options) const = 0;
};

// Construct a registered builtin Any/ArrayAny by its class id WITHOUT instantiating the sibling
// template here (definition in intf_object_registry.h, where the registry is visible).
IAny::Ptr ConstructRegisteredAny(const ObjectId& id);

template<typename Type>
class BaseTypedAny : public AnyBase {
public:
    static constexpr TypeId TYPE_ID = UidFromType<Type>();
    static constexpr bool IS_PTR_TYPE = IsInterfacePtr_v<Type>;

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
        if constexpr (IS_PTR_TYPE) {
            return AnyPC<Type>::template GetCompatibleTypes<Type>(dir);
        }

        static constexpr TypeId uids[] = {TYPE_ID};
        return uids;
    }

    TypeId GetTypeId() const
    {
        return TYPE_ID;
    }

protected:
    static constexpr AnyTypeInfo TYPE_INFO {
        StaticGetClassId(),
        TYPE_ID,
        ArrayUidFromType<Type>(),
        ItemUidFromType<Type>(),
        StaticGetTypeIdString(),
        sizeof(Type), /*NOLINT(bugprone-sizeof-expression)*/
        &StaticGetCompatibleTypes,
    };
    const AnyTypeInfo& Info() const override
    {
        return TYPE_INFO;
    }
    AnyReturnValue RawGet(void* data) const override
    {
        *static_cast<Type*>(data) = InternalGetValue();
        return AnyReturn::SUCCESS;
    }
    AnyReturnValue RawSet(const void* data) override
    {
        return InternalSetValue(*static_cast<const Type*>(data));
    }
    AnyReturnValue RawGetCompat(const TypeId& id, void* data, size_t size) const override
    {
        if constexpr (IS_PTR_TYPE) {
            if (data && AnyPC<Type>::IsValidGetArgs(id, data, size)) {
                using IIType = typename AnyPC<Type>::IIType;
                return AnyPC<Type>::GetData(id, data, interface_pointer_cast<IIType>(InternalGetValue()));
            }
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue RawSetCompat(const TypeId& id, const void* data, size_t size) override
    {
        if constexpr (IS_PTR_TYPE) {
            if (data && AnyPC<Type>::IsValidSetArgs(id, data, size)) {
                typename AnyPC<Type>::IIPtrType p;
                if (AnyPC<Type>::SetData(id, data, p)) {
                    if (auto ptr = interface_pointer_cast<typename Type::element_type>(p); ptr || !p) {
                        return InternalSetValue(ptr);
                    }
                }
                // Matches the original best-effort fall-through: the id is a compatible pointer id but
                // the value is not castable to the target interface; store the punned value anyway.
                return InternalSetValue(*static_cast<const Type*>(data));
            }
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue RawCopyFrom(const IAny& any) override
    {
        if constexpr (IS_PTR_TYPE) {
            typename AnyPC<Type>::IIPtrType p;
            if (any.GetValue(p)) {
                if (auto ptr = interface_pointer_cast<typename Type::element_type>(p); ptr || !p) {
                    return InternalSetValue(ptr);
                }
            }
        } else {
            Type value;
            if (any.GetValue(value)) {
                return InternalSetValue(value);
            }
        }
        return AnyReturn::FAIL;
    }
    AnyReturnValue RawReset() override
    {
        return InternalSetValue(Type {});
    }

    virtual AnyReturnValue InternalSetValue(const Type& value) = 0;
    virtual const Type& InternalGetValue() const = 0;
};

template<typename T, bool Compare = HasEqualOperator_v<T>>
struct DefaultCompare {
    static constexpr bool Equal(const T& v1, const T& v2)
    {
        if constexpr (Compare) {
            return v1 == v2;
        } else {
            return false;
        }
    }

    template<typename NewType>
    using Rebind = DefaultCompare<NewType>;
};

template<typename Type, typename Compare>
class ArrayAny;

/**
 * @brief Default IAny implementation which supports a single type.
 */
template<typename Type, typename Compare = DefaultCompare<Type>>
class Any : public BaseTypedAny<Type> {
    using Super = BaseTypedAny<Type>;

public:
    explicit Any(Type v = {}) : value_(BASE_NS::move(v))
    {}
    AnyReturnValue InternalSetValue(const Type& value) override
    {
        if (!Compare::Equal(value, value_)) {
            value_ = value;
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::NOTHING_TO_DO;
    }
    const Type& InternalGetValue() const override
    {
        return value_;
    }
    using Super::Clone;
    IAny::Ptr Clone(bool withValue) const
    {
        return Clone({withValue ? CloneValueType::COPY_VALUE : CloneValueType::DEFAULT_VALUE});
    }

    bool operator==(const Any<Type>& other) const noexcept
    {
        return (Compare::Equal(other.InternalGetValue(), value_));
    }

protected:
    IAny::Ptr RawClone(const AnyCloneOptions& options) const override
    {
        if (options.role == TypeIdRole::ARRAY) {
            return ConstructRegisteredAny(MakeUid<Type>(BUILTIN_ARRAY_ANY_TAG));
        }
        return IAny::Ptr(new Any {options.value == CloneValueType::COPY_VALUE ? value_ : Type {}});
    }

private:
    Type value_;
};

/**
 * @brief Per-type identity descriptor for the builtin array Any implementations.
 */
struct ArrayAnyTypeInfo {
    ObjectId classId;
    TypeId arrayId;
    TypeId itemId;
    TypeId vectorId;
    BASE_NS::string_view typeName;
    size_t vectorSize;
    size_t itemSize;
    const BASE_NS::array_view<const TypeId> (*getCompatibleTypes)(CompatibilityDirection);
    const BASE_NS::array_view<const TypeId> (*getItemCompatibleTypes)(CompatibilityDirection);
};

/**
 * @brief Type-erased base shared by all builtin array Any types.
 *
 * Implements the type-independent IArrayAny/IValue surface once, driven by a per-type descriptor +
 * small element/vector hooks the typed subclass supplies. Only the genuinely per-element-type vector
 * operations remain instantiated per type.
 */
class ArrayAnyBase : public IntroduceInterfaces<IArrayAny, IValue> {
public:
    ObjectId GetClassId() const override
    {
        return Info().classId;
    }
    const BASE_NS::array_view<const TypeId> GetCompatibleTypes(CompatibilityDirection dir) const override
    {
        return Info().getCompatibleTypes(dir);
    }
    const BASE_NS::array_view<const TypeId> GetItemCompatibleTypes(CompatibilityDirection dir) const override
    {
        return Info().getItemCompatibleTypes(dir);
    }
    AnyReturnValue GetData(const TypeId& id, void* data, size_t size) const override
    {
        const auto& info = Info();
        if (data && size == info.vectorSize && id == info.vectorId) {
            RawGetVector(data);
            return AnyReturn::SUCCESS;
        }
        if (data && id == info.arrayId && size >= info.itemSize) {
            const void* ptr = nullptr;
            size_t count = 0;
            RawData(ptr, count);
            const auto valueSize = count * info.itemSize;
            if (size >= valueSize) {
                BASE_NS::CloneData(data, size, ptr, valueSize);
                return AnyReturn::SUCCESS;
            }
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue SetData(const TypeId& id, const void* data, size_t size) override
    {
        const auto& info = Info();
        if (data && size == info.vectorSize && id == info.vectorId) {
            return RawSetVector(data);
        }
        if (data && id == info.arrayId && size >= info.itemSize) {
            return RawSetFromArray(data, size / info.itemSize);
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue CopyFrom(const IAny& any) override
    {
        return RawCopyFrom(any);
    }
    AnyReturnValue ResetValue() override
    {
        return RawReset();
    }
    IAny::Ptr Clone(const AnyCloneOptions& options) const override
    {
        return RawClone(options);
    }
    TypeId GetTypeId(TypeIdRole role) const override
    {
        const auto& info = Info();
        if (role == TypeIdRole::ITEM) {
            return info.itemId;
        }
        return info.arrayId;
    }
    BASE_NS::string GetTypeIdString() const override
    {
        return BASE_NS::string(Info().typeName);
    }
    using IAny::SetValue;
    AnyReturnValue SetValue(const IAny& value) override
    {
        return CopyFrom(value);
    }
    using IAny::GetValue;
    const IAny& GetValue() const override
    {
        return *this;
    }
    bool IsCompatible(const TypeId& id) const override
    {
        return META_NS::IsCompatible(*this, id);
    }
    AnyReturnValue GetDataAt(size_t index, const TypeId& id, void* data, size_t size) const override
    {
        const auto& info = Info();
        if (data && size == info.itemSize && id == info.itemId && index < RawSize()) {
            return RawGetItem(index, data);
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue SetDataAt(size_t index, const TypeId& id, const void* data, size_t size) override
    {
        const auto& info = Info();
        if (data && size == info.itemSize && id == info.itemId && index < RawSize()) {
            return RawSetItem(index, data);
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue RemoveAt(IArrayAny::IndexType index) override
    {
        return RawRemoveAt(index);
    }
    void RemoveAll() override
    {
        RawRemoveAll();
    }
    IArrayAny::IndexType GetSize() const noexcept override
    {
        return RawSize();
    }

protected:
    virtual const ArrayAnyTypeInfo& Info() const = 0;
    virtual void RawGetVector(void* out) const = 0;
    virtual AnyReturnValue RawSetVector(const void* in) = 0;
    virtual AnyReturnValue RawSetFromArray(const void* data, size_t count) = 0;
    virtual void RawData(const void*& ptr, size_t& count) const = 0;
    virtual IArrayAny::IndexType RawSize() const = 0;
    virtual AnyReturnValue RawGetItem(size_t index, void* out) const = 0;
    virtual AnyReturnValue RawSetItem(size_t index, const void* in) = 0;
    virtual AnyReturnValue RawRemoveAt(size_t index) = 0;
    virtual void RawRemoveAll() = 0;
    virtual AnyReturnValue RawCopyFrom(const IAny& any) = 0;
    virtual AnyReturnValue RawReset() = 0;
    virtual IAny::Ptr RawClone(const AnyCloneOptions& options) const = 0;
};

template<typename Type>
class BaseTypedArrayAny : public ArrayAnyBase {
public:
    static constexpr TypeId VECTOR_TYPE_ID = UidFromType<BASE_NS::vector<Type>>();
    static constexpr TypeId ARRAY_TYPE_ID = ArrayUidFromType<Type>();
    static constexpr TypeId ITEM_TYPE_ID = ItemUidFromType<Type>();
    static constexpr TypeId TYPE_ID = ARRAY_TYPE_ID;
    using ItemType = Type;
    using ArrayType = BASE_NS::vector<Type>;

    static constexpr ObjectId StaticGetClassId()
    {
        return MakeUid<Type>(BUILTIN_ARRAY_ANY_TAG);
    }
    static constexpr BASE_NS::string_view StaticGetTypeIdString()
    {
        return MetaType<ArrayType>::name;
    }
    static const BASE_NS::array_view<const TypeId> StaticGetCompatibleTypes(CompatibilityDirection)
    {
        static constexpr TypeId uids[] = {ARRAY_TYPE_ID, VECTOR_TYPE_ID};
        return uids;
    }
    TypeId GetTypeId() const
    {
        return TYPE_ID;
    }

protected:
    static constexpr ArrayAnyTypeInfo TYPE_INFO {
        StaticGetClassId(),
        ARRAY_TYPE_ID,
        ITEM_TYPE_ID,
        VECTOR_TYPE_ID,
        StaticGetTypeIdString(),
        sizeof(ArrayType),
        sizeof(Type), /*NOLINT(bugprone-sizeof-expression)*/
        &StaticGetCompatibleTypes,
        &BaseTypedAny<Type>::StaticGetCompatibleTypes,
    };
    const ArrayAnyTypeInfo& Info() const override
    {
        return TYPE_INFO;
    }
    void RawGetVector(void* out) const override
    {
        *static_cast<ArrayType*>(out) = InternalGetValue();
    }
    void RawData(const void*& ptr, size_t& count) const override
    {
        const auto& value = InternalGetValue();
        ptr = value.data();
        count = value.size();
    }
    IArrayAny::IndexType RawSize() const override
    {
        return InternalGetValue().size();
    }
    AnyReturnValue RawGetItem(size_t index, void* out) const override
    {
        *static_cast<Type*>(out) = InternalGetValue()[index];
        return AnyReturn::SUCCESS;
    }
    AnyReturnValue RawSetVector(const void* in) override
    {
        return InternalSetValue(*static_cast<const ArrayType*>(in));
    }
    AnyReturnValue RawSetFromArray(const void* data, size_t count) override
    {
        auto p = static_cast<const Type*>(data);
        return InternalSetValue(ArrayType(p, p + count));
    }
    AnyReturnValue RawCopyFrom(const IAny& any) override
    {
        ArrayType value;
        if (any.GetValue(value)) {
            return InternalSetValue(value);
        }
        return AnyReturn::FAIL;
    }
    AnyReturnValue RawReset() override
    {
        return InternalSetValue(ArrayType {});
    }

    virtual AnyReturnValue InternalSetValue(const ArrayType& value) = 0;
    virtual const ArrayType& InternalGetValue() const = 0;
};

/**
 * @brief Default IArrayAny implementation which supports a single type.
 */
template<typename Type, typename Compare = DefaultCompare<BASE_NS::vector<Type>>>
class ArrayAny : public BaseTypedArrayAny<Type> {
    using Super = BaseTypedArrayAny<Type>;

public:
    using ArrayType = typename Super::ArrayType;
    using ItemType = typename Super::ItemType;
    using Super::ITEM_TYPE_ID;
    static constexpr auto ITEM_SIZE = sizeof(ItemType); /*NOLINT(bugprone-sizeof-expression)*/

    explicit constexpr ArrayAny(ArrayType v = {}) : value_(BASE_NS::move(v))
    {}
    explicit constexpr ArrayAny(const BASE_NS::array_view<const Type>& v) : value_(v.begin(), v.end())
    {}
#ifdef BASE_VECTOR_HAS_INITIALIZE_LIST
    constexpr ArrayAny(std::initializer_list<Type> v) : value_(ArrayType(v))
    {}
#endif
    AnyReturnValue SetAnyAt(IArrayAny::IndexType index, const IAny& value) override
    {
        ItemType v;
        if (value.GetData(ITEM_TYPE_ID, &v, ITEM_SIZE)) {
            return this->SetDataAt(index, ITEM_TYPE_ID, &v, ITEM_SIZE);
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue GetAnyAt(IArrayAny::IndexType index, IAny& value) const override
    {
        ItemType v;
        if (this->GetDataAt(index, ITEM_TYPE_ID, &v, ITEM_SIZE)) {
            return value.SetData(ITEM_TYPE_ID, &v, ITEM_SIZE);
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue InsertAnyAt(IArrayAny::IndexType index, const IAny& value) override
    {
        ItemType v;
        if (value.GetData(ITEM_TYPE_ID, &v, ITEM_SIZE)) {
            index = index < value_.size() ? index : value_.size();
            value_.insert(value_.begin() + index, v);
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    using Super::Clone;
    IAny::Ptr Clone(bool withValue) const
    {
        return Clone({withValue ? CloneValueType::COPY_VALUE : CloneValueType::DEFAULT_VALUE});
    }
    const ArrayType& InternalGetValue() const override
    {
        return value_;
    }
    void PushBack(ItemType item)
    {
        value_.push_back(BASE_NS::move(item));
    }

protected:
    AnyReturnValue RawSetItem(size_t index, const void* in) override
    {
        value_[index] = *static_cast<const ItemType*>(in);
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
    IAny::Ptr RawClone(const AnyCloneOptions& options) const override
    {
        if (options.role == TypeIdRole::ITEM) {
            return IAny::Ptr(new Any<Type, typename Compare::template Rebind<Type>>());
        }
        return IAny::Ptr(new ArrayAny {options.value == CloneValueType::COPY_VALUE ? value_ : ArrayType {}});
    }

private:
    AnyReturnValue InternalSetValue(const ArrayType& value) override
    {
        if (!Compare::Equal(value, value_)) {
            value_ = value;
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::NOTHING_TO_DO;
    }

protected:
    ArrayType value_;
};

META_END_NAMESPACE()

#endif
