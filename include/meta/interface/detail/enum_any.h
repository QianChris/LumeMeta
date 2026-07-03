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
#ifndef META_INTERFACE_DETAIL_ENUM_ANY_H
#define META_INTERFACE_DETAIL_ENUM_ANY_H

#include <meta/interface/detail/base_any.h>
#include <meta/interface/detail/multi_type_any.h>

META_BEGIN_NAMESPACE()

// Widened compatibility type for an enum: its underlying integer promoted to 64-bit, keeping signedness.
template<typename T>
using EnumCompatType = BASE_NS::conditional_t<BASE_NS::is_unsigned_v<BASE_NS::underlying_type_t<T>>, uint64_t, int64_t>;

/// An integer-compatible type an enum any converts to/from (its uid + width + signedness). Used by the
/// Core enum anys (ext/engine/core_enum_any.h), which can have several compat types.
struct EnumCompatEntry {
    TypeId uid;
    size_t size;
    bool isUnsigned;
};

namespace Internal {

// Low `size`-byte mask (size in bytes; size >= 8 -> all bits).
inline constexpr uint64_t EnumBitMask(size_t size)
{
    return size >= sizeof(uint64_t) ? ~uint64_t{0} : ((uint64_t{1} << (size * 8)) - 1);
}

// Sign/zero-extend a raw `size`-byte value (held in the low bytes of `bits`) to a full 64-bit value,
// matching what static_cast<WiderInt>(enumValue) would produce.
inline constexpr uint64_t EnumExtend(uint64_t bits, size_t size, bool isUnsigned)
{
    bits &= EnumBitMask(size);
    if (!isUnsigned && size < sizeof(uint64_t)) {
        const uint64_t signBit = uint64_t{1} << (size * 8 - 1);
        if (bits & signBit) {
            bits |= ~EnumBitMask(size);
        }
    }
    return bits;
}

/**
 * @brief Per-enum identity + marshalling descriptor for the shared scalar enum Any. Only DATA differs per
 *        enum type; the code/vtable is shared (see EnumAnyImpl). Mirrors InterfacePtrAnyInfo.
 */
struct EnumAnyInfo {
    AnyTypeInfo any;        // any.size == the enum's underlying integer size (1/2/4/8)
    ObjectId arrayClassId;  // for the scalar->array Clone
    bool isSigned;          // underlying signedness -> sign-extend on read/marshal
    TypeId compatId;        // the widened compat id (int64_t / uint64_t)
};

/**
 * @brief One shared implementation for all plain (non-metadata) enum scalar Anys. Non-templated: the enum
 *        identity and marshalling width are carried per-INSTANCE via an EnumAnyInfo pointer, so the vtable
 *        + method bodies fold to a single copy instead of a distinct MultiTypeAny<Enum> per enum type.
 *        Storage is the enum value sign-extended into a uint64_t (little-endian hosts: ARM64 / x64).
 */
class EnumAnyImpl : public AnyBase {
public:
    EnumAnyImpl(const EnumAnyInfo* info, uint64_t value) : info_(info), value_(value)
    {}

protected:
    const AnyTypeInfo& Info() const override
    {
        return info_->any;
    }
    AnyReturnValue Store(uint64_t bits)
    {
        bits = EnumExtend(bits, info_->any.size, !info_->isSigned);
        if (bits != value_) {
            value_ = bits;
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::NOTHING_TO_DO;
    }
    static uint64_t LoadBytes(const void* data, size_t size)
    {
        uint64_t bits = 0;
        BASE_NS::CloneData(&bits, sizeof(bits), data, size);  // low `size` bytes (little-endian)
        return bits;
    }

    AnyReturnValue RawGet(void* data) const override
    {
        BASE_NS::CloneData(data, info_->any.size, &value_, info_->any.size);
        return AnyReturn::SUCCESS;
    }
    AnyReturnValue RawSet(const void* data) override
    {
        return Store(LoadBytes(data, info_->any.size));
    }
    AnyReturnValue RawGetCompat(const TypeId& id, void* data, size_t size) const override
    {
        if (data && size == sizeof(uint64_t) && id == info_->compatId) {
            BASE_NS::CloneData(data, size, &value_, sizeof(value_));  // memcpy: no alignment assumption on data
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue RawSetCompat(const TypeId& id, const void* data, size_t size) override
    {
        if (data && size == sizeof(uint64_t) && id == info_->compatId) {
            uint64_t v = 0;
            BASE_NS::CloneData(&v, sizeof(v), data, size);
            return Store(v);
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue RawCopyFrom(const IAny& any) override
    {
        if (META_NS::IsCompatible(any, info_->any.typeId, CompatibilityDirection::GET)) {
            alignas(uint64_t) uint8_t tmp[sizeof(uint64_t)] = {};
            if (any.GetData(info_->any.typeId, tmp, info_->any.size)) {
                return Store(LoadBytes(tmp, info_->any.size));
            }
        }
        if (META_NS::IsCompatible(any, info_->compatId, CompatibilityDirection::GET)) {
            uint64_t v = 0;
            if (any.GetData(info_->compatId, &v, sizeof(v))) {
                return Store(v);
            }
        }
        return AnyReturn::FAIL;
    }
    AnyReturnValue RawReset() override
    {
        return Store(0);
    }
    IAny::Ptr RawClone(const AnyCloneOptions& options) const override
    {
        if (options.role == TypeIdRole::ARRAY) {
            return ConstructRegisteredAny(info_->arrayClassId);
        }
        return IAny::Ptr(new EnumAnyImpl(info_, options.value == CloneValueType::COPY_VALUE ? value_ : 0));
    }

    const EnumAnyInfo* info_;
    uint64_t value_;
};

/**
 * @brief Thin per-enum facade so AnyType<Enum> resolves to a concrete constructible type and registration
 *        keeps the per-type class id / name. Adds no virtuals, so its vtable is identical to EnumAnyImpl's
 *        and folds to a single copy; the only per-type residue is the static descriptor (data).
 */
template<typename Type>
class EnumAny : public EnumAnyImpl {
public:
    using Underlying = BASE_NS::underlying_type_t<Type>;
    static constexpr TypeId TYPE_ID = UidFromType<Type>();
    static constexpr TypeId COMPAT_ID = UidFromType<META_NS::EnumCompatType<Type>>();

    static constexpr ObjectId StaticGetClassId()
    {
        // Preserve the class id the plain-enum path used before (MultiTypeAny::StaticGetClassId), so
        // existing registrations and serialised any ids keep resolving.
        return MakeUid<Type>(MULTI_TYPE_ANY_TAG);
    }
    static constexpr BASE_NS::string_view StaticGetTypeIdString()
    {
        return MetaType<Type>::name;
    }
    static const BASE_NS::array_view<const TypeId> StaticGetCompatibleTypes(CompatibilityDirection)
    {
        static constexpr TypeId ids[] = {TYPE_ID, COMPAT_ID};
        return ids;
    }

    static constexpr EnumAnyInfo INFO{
        AnyTypeInfo{
            StaticGetClassId(),
            TYPE_ID,
            ArrayUidFromType<Type>(),
            ItemUidFromType<Type>(),
            StaticGetTypeIdString(),
            sizeof(Type),
            &StaticGetCompatibleTypes,
        },
        MakeUid<Type>(MULTI_TYPE_ARRAY_ANY_TAG),
        !BASE_NS::is_unsigned_v<Underlying>,
        COMPAT_ID,
    };

    explicit EnumAny(Type v = {}) : EnumAnyImpl(&INFO, Widen(v))
    {}

    // Concrete-typed read-back for generic callers using the Any<T> fast-path.
    Type InternalGetValue() const
    {
        Type v{};
        this->GetData(TYPE_ID, &v, sizeof(Type));
        return v;
    }
    TypeId GetTypeId() const
    {
        return TYPE_ID;
    }

private:
    static uint64_t Widen(Type v)
    {
        if constexpr (BASE_NS::is_unsigned_v<Underlying>) {
            return static_cast<uint64_t>(static_cast<Underlying>(v));
        } else {
            return static_cast<uint64_t>(static_cast<int64_t>(static_cast<Underlying>(v)));
        }
    }
};

/**
 * @brief Shared byte-buffer array-any storage for enum-like element types. Elements are trivially-copyable
 *        integers stored contiguously at stride == Info().itemSize, so every element/array path is a plain
 *        byte copy; only the concrete vector<ElemT> boundary needs the three per-type thunks (supplied at
 *        construction). Concrete subclasses (ArrayEnumAnyImpl here, ArrayCoreEnumAnyImpl in
 *        ext/engine/core_enum_any.h) add only RawClone. Mirrors the scalar sharing.
 */
class ByteArrayAnyImpl : public ArrayAnyBase {
public:
    using BytesToVector = void (*)(const uint8_t* bytes, size_t count, void* outVec);
    using VectorToBytes = void (*)(const void* inVec, BASE_NS::vector<uint8_t>& outBytes);
    using AnyToBytes = bool (*)(const IAny& any, BASE_NS::vector<uint8_t>& outBytes);

    ByteArrayAnyImpl(const ArrayAnyTypeInfo* array, BytesToVector bytesToVector, VectorToBytes vectorToBytes,
        AnyToBytes anyToBytes, BASE_NS::vector<uint8_t> bytes)
        : array_(array),
          bytesToVector_(bytesToVector),
          vectorToBytes_(vectorToBytes),
          anyToBytes_(anyToBytes),
          bytes_(BASE_NS::move(bytes))
    {}

    AnyReturnValue SetAnyAt(IArrayAny::IndexType index, const IAny& value) override
    {
        alignas(uint64_t) uint8_t tmp[sizeof(uint64_t)] = {};
        if (value.GetData(array_->itemId, tmp, ItemSize())) {
            return SetDataAt(index, array_->itemId, tmp, ItemSize());
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue GetAnyAt(IArrayAny::IndexType index, IAny& value) const override
    {
        alignas(uint64_t) uint8_t tmp[sizeof(uint64_t)] = {};
        if (GetDataAt(index, array_->itemId, tmp, ItemSize())) {
            return value.SetData(array_->itemId, tmp, ItemSize());
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue InsertAnyAt(IArrayAny::IndexType index, const IAny& value) override
    {
        alignas(uint64_t) uint8_t tmp[sizeof(uint64_t)] = {};
        const size_t sz = ItemSize();
        if (value.GetData(array_->itemId, tmp, sz)) {
            const size_t count = bytes_.size() / sz;
            index = index < count ? index : count;
            bytes_.insert(bytes_.begin() + index * sz, tmp, tmp + sz);
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INVALID_ARGUMENT;
    }

protected:
    const ArrayAnyTypeInfo& Info() const override
    {
        return *array_;
    }
    size_t ItemSize() const
    {
        return array_->itemSize;
    }
    IArrayAny::IndexType Count() const
    {
        return bytes_.size() / ItemSize();
    }
    const BASE_NS::vector<uint8_t>& Bytes() const
    {
        return bytes_;
    }

    void RawGetVector(void* out) const override
    {
        bytesToVector_(bytes_.data(), Count(), out);
    }
    AnyReturnValue RawSetVector(const void* in) override
    {
        BASE_NS::vector<uint8_t> next;
        vectorToBytes_(in, next);
        return Assign(BASE_NS::move(next));
    }
    AnyReturnValue RawSetFromArray(const void* data, size_t count) override
    {
        const size_t bytes = count * ItemSize();
        BASE_NS::vector<uint8_t> next(bytes);
        if (bytes) {
            BASE_NS::CloneData(next.data(), bytes, data, bytes);
        }
        return Assign(BASE_NS::move(next));
    }
    void RawData(const void*& ptr, size_t& count) const override
    {
        ptr = bytes_.data();
        count = Count();
    }
    IArrayAny::IndexType RawSize() const override
    {
        return Count();
    }
    AnyReturnValue RawGetItem(size_t index, void* out) const override
    {
        const size_t sz = ItemSize();
        BASE_NS::CloneData(out, sz, bytes_.data() + index * sz, sz);
        return AnyReturn::SUCCESS;
    }
    AnyReturnValue RawSetItem(size_t index, const void* in) override
    {
        const size_t sz = ItemSize();
        BASE_NS::CloneData(bytes_.data() + index * sz, sz, in, sz);
        return AnyReturn::SUCCESS;
    }
    AnyReturnValue RawRemoveAt(size_t index) override
    {
        const size_t sz = ItemSize();
        if (index < Count()) {
            bytes_.erase(bytes_.begin() + index * sz, bytes_.begin() + (index + 1) * sz);
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    void RawRemoveAll() override
    {
        bytes_.clear();
    }
    AnyReturnValue RawCopyFrom(const IAny& any) override
    {
        BASE_NS::vector<uint8_t> next;
        if (!anyToBytes_(any, next)) {
            return AnyReturn::FAIL;
        }
        return Assign(BASE_NS::move(next));
    }
    AnyReturnValue RawReset() override
    {
        if (bytes_.empty()) {
            return AnyReturn::NOTHING_TO_DO;
        }
        bytes_.clear();
        return AnyReturn::SUCCESS;
    }

private:
    AnyReturnValue Assign(BASE_NS::vector<uint8_t> next)
    {
        if (next.size() == bytes_.size()) {
            bool equal = true;
            for (size_t i = 0; equal && i != next.size(); ++i) {
                equal = next[i] == bytes_[i];
            }
            if (equal) {
                return AnyReturn::NOTHING_TO_DO;
            }
        }
        bytes_ = BASE_NS::move(next);
        return AnyReturn::SUCCESS;
    }

    const ArrayAnyTypeInfo* array_;
    BytesToVector bytesToVector_;
    VectorToBytes vectorToBytes_;
    AnyToBytes anyToBytes_;
    BASE_NS::vector<uint8_t> bytes_;
};

/**
 * @brief Per-enum identity descriptor for the shared plain-enum array Any.
 */
struct ArrayEnumAnyInfo {
    ArrayAnyTypeInfo array;
    const EnumAnyInfo* scalar;  // for the array->item Clone (constructs the shared scalar)
    // The only per-enum-type residue: the vector<Type> boundary thunks.
    ByteArrayAnyImpl::BytesToVector getVector;
    ByteArrayAnyImpl::VectorToBytes setVector;
    ByteArrayAnyImpl::AnyToBytes copyFrom;
};

/// Plain-enum array any: shared byte-buffer storage + an item/array Clone building the plain scalar.
class ArrayEnumAnyImpl : public ByteArrayAnyImpl {
public:
    ArrayEnumAnyImpl(const ArrayEnumAnyInfo* info, BASE_NS::vector<uint8_t> bytes)
        : ByteArrayAnyImpl(&info->array, info->getVector, info->setVector, info->copyFrom, BASE_NS::move(bytes)),
          info_(info)
    {}

protected:
    IAny::Ptr RawClone(const AnyCloneOptions& options) const override
    {
        if (options.role == TypeIdRole::ITEM) {
            return IAny::Ptr(new EnumAnyImpl(info_->scalar, 0));
        }
        return IAny::Ptr(new ArrayEnumAnyImpl(
            info_, options.value == CloneValueType::COPY_VALUE ? Bytes() : BASE_NS::vector<uint8_t>{}));
    }

private:
    const ArrayEnumAnyInfo* info_;
};

/**
 * @brief Thin per-enum facade for the array any (see EnumAny for the scalar rationale).
 */
template<typename Type>
class EnumArrayAny : public ArrayEnumAnyImpl {
public:
    using Underlying = BASE_NS::underlying_type_t<Type>;
    using ArrayType = BASE_NS::vector<Type>;
    static constexpr TypeId ARRAY_TYPE_ID = ArrayUidFromType<Type>();
    static constexpr TypeId VECTOR_TYPE_ID = UidFromType<BASE_NS::vector<Type>>();

    static constexpr ObjectId StaticGetClassId()
    {
        return MakeUid<Type>(MULTI_TYPE_ARRAY_ANY_TAG);
    }
    static constexpr BASE_NS::string_view StaticGetTypeIdString()
    {
        return MetaType<BASE_NS::vector<Type>>::name;
    }
    static const BASE_NS::array_view<const TypeId> StaticGetCompatibleTypes(CompatibilityDirection)
    {
        static constexpr TypeId ids[] = {ARRAY_TYPE_ID, VECTOR_TYPE_ID};
        return ids;
    }
    static const BASE_NS::array_view<const TypeId> StaticGetItemCompatibleTypes(CompatibilityDirection dir)
    {
        return EnumAny<Type>::StaticGetCompatibleTypes(dir);
    }

    static void GetVectorThunk(const uint8_t* buf, size_t count, void* outVec)
    {
        auto* v = static_cast<ArrayType*>(outVec);
        v->resize(count);
        if (count) {
            BASE_NS::CloneData(v->data(), count * sizeof(Type), buf, count * sizeof(Type));
        }
    }
    static void SetVectorThunk(const void* inVec, BASE_NS::vector<uint8_t>& buf)
    {
        const auto* v = static_cast<const ArrayType*>(inVec);
        buf.resize(v->size() * sizeof(Type));
        if (!buf.empty()) {
            BASE_NS::CloneData(buf.data(), buf.size(), v->data(), buf.size());
        }
    }
    static bool CopyFromThunk(const IAny& any, BASE_NS::vector<uint8_t>& buf)
    {
        ArrayType v;
        if (any.GetValue(v)) {
            SetVectorThunk(&v, buf);
            return true;
        }
        return false;
    }

    static constexpr ArrayEnumAnyInfo INFO{
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
        &EnumAny<Type>::INFO,
        &GetVectorThunk,
        &SetVectorThunk,
        &CopyFromThunk,
    };

    explicit EnumArrayAny(ArrayType v = {}) : ArrayEnumAnyImpl(&INFO, ToBuffer(v))
    {}
    explicit EnumArrayAny(const BASE_NS::array_view<const Type>& v) : ArrayEnumAnyImpl(&INFO, ToBuffer(v))
    {}

    BASE_NS::vector<Type> InternalGetValue() const
    {
        BASE_NS::vector<Type> v;
        this->GetData(VECTOR_TYPE_ID, &v, sizeof(v));
        return v;
    }
    TypeId GetTypeId() const
    {
        return ARRAY_TYPE_ID;
    }

private:
    template<typename Range>
    static BASE_NS::vector<uint8_t> ToBuffer(const Range& v)
    {
        BASE_NS::vector<uint8_t> buf(v.size() * sizeof(Type));
        if (!buf.empty()) {
            BASE_NS::CloneData(buf.data(), buf.size(), v.data(), buf.size());
        }
        return buf;
    }
};

}  // namespace Internal

// Enum types map to Internal::EnumAny / Internal::EnumArrayAny via MapAnyType (see detail/any.h).

META_END_NAMESPACE()

#endif
