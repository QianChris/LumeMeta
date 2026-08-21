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
#ifndef META_EXT_ENGINE_CORE_ENUM_ANY_H
#define META_EXT_ENGINE_CORE_ENUM_ANY_H

#include <meta/base/type_traits.h>
#include <meta/interface/detail/enum_any.h>
#include <meta/interface/engine/intf_engine_type.h>
#include <meta/interface/intf_enum.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Per-enum identity descriptor for the generic Core enum scalar any. Only DATA differs per enum type
 *        (uids/size/signedness + the compat list + the array-clone thunk); the code/vtable is shared
 *        (see CoreEnumAnyImpl), so it folds to a single copy instead of a CoreEnumAny<EnumT> (+ its
 *        MultiTypeAny<EnumT>) instantiation per enum type.
 */
struct CoreEnumAnyInfo {
    AnyTypeInfo any;                                    // size == the enum's underlying size
    bool isUnsigned;                                    // underlying signedness (for compat sign-extension)
    BASE_NS::array_view<const EnumCompatEntry> compat;  // e.g. {..CoreType.., int64_t}
    IAny::Ptr (*makeArray)(const CORE_NS::Property&);   // scalar->array Clone (builds ArrayCoreEnumAny<EnumT>(prop))
};

/// Per-enum identity descriptor for the generic Core enum array any (see CoreEnumAnyInfo).
struct ArrayCoreEnumAnyInfo {
    ArrayAnyTypeInfo array;                                         // itemSize == the enum's underlying size
    const CoreEnumAnyInfo* scalar;                                  // for array->item Clone (builds the scalar impl)
    void (*bytesToVector)(const uint8_t*, size_t count, void*);     // RawGetVector: bytes -> vector<EnumT>
    void (*vectorToBytes)(const void*, BASE_NS::vector<uint8_t>&);  // RawSetVector: vector<EnumT> -> bytes
    bool (*anyToBytes)(const IAny&, BASE_NS::vector<uint8_t>&);     // RawCopyFrom: extract vector<EnumT> from any
};

/**
 * @brief One shared implementation for all Core enum scalar anys, parameterised only by a per-INSTANCE
 *        CoreEnumAnyInfo. An enum value is just an integer, and the IEnum/IEngineType metadata is read live
 *        from the CORE_NS::Property, so nothing here is per-enum-type: the value is stored as raw bits and
 *        marshalled by the descriptor's size; the enum member table comes from prop_.metaData.enumMetaData.
 */
class CoreEnumAnyImpl : public IntroduceInterfaces<AnyBase, IEnum, IEngineType> {
public:
    CoreEnumAnyImpl(const CoreEnumAnyInfo* info, const CORE_NS::Property& p, uint64_t bits = 0)
        : info_(info),
          prop_(p),
          type_(p.flags & uint32_t(CORE_NS::PropertyFlags::IS_BITFIELD) ? META_NS::EnumType::BIT_FIELD
                                                                        : META_NS::EnumType::SINGLE_VALUE),
          bits_(bits & Internal::EnumBitMask(info->any.size))
    {}

    // IEngineType
    CORE_NS::PropertyTypeDecl GetTypeDecl() const override
    {
        return prop_.type;
    }

    // IInfo
    BASE_NS::string_view GetName() const override
    {
        return prop_.name;
    }
    BASE_NS::string_view GetDescription() const override
    {
        return prop_.displayName;
    }

    // IEnum
    META_NS::EnumType GetEnumType() const override
    {
        return type_;
    }
    BASE_NS::vector<EnumValue> GetValues() const override
    {
        BASE_NS::vector<EnumValue> values;
        values.reserve(prop_.metaData.enumMetaData.size());
        for (auto&& v : prop_.metaData.enumMetaData) {
            values.push_back(EnumValue{v.name, v.displayName});
        }
        return values;
    }
    size_t GetValueIndex() const override
    {
        return IndexOfBits(bits_);
    }
    bool SetValueIndex(size_t index) override
    {
        if (index < prop_.metaData.enumMetaData.size()) {
            bits_ = uint64_t(prop_.metaData.enumMetaData[index].value) & Mask();
            return true;
        }
        return false;
    }
    bool IsValueSet(size_t index) const override
    {
        if (index < prop_.metaData.enumMetaData.size()) {
            const uint64_t mv = uint64_t(prop_.metaData.enumMetaData[index].value) & Mask();
            if (type_ == META_NS::EnumType::BIT_FIELD) {
                return (bits_ & mv) != 0;
            }
            return bits_ == mv;
        }
        return false;
    }
    bool FlipValue(size_t index, bool isSet) override
    {
        if (type_ == META_NS::EnumType::BIT_FIELD && index < prop_.metaData.enumMetaData.size()) {
            const uint64_t mv = uint64_t(prop_.metaData.enumMetaData[index].value) & Mask();
            bits_ = (isSet ? (bits_ | mv) : (bits_ & ~mv)) & Mask();
            return true;
        }
        return false;
    }

    using IAny::Clone;

protected:
    const AnyTypeInfo& Info() const override
    {
        return info_->any;
    }
    AnyReturnValue RawGet(void* data) const override
    {
        BASE_NS::CloneData(data, info_->any.size, &bits_, info_->any.size);  // low bytes == the enum value
        return AnyReturn::SUCCESS;
    }
    AnyReturnValue RawSet(const void* data) override
    {
        uint64_t v = 0;
        BASE_NS::CloneData(&v, sizeof(v), data, info_->any.size);
        return SetBits(v);
    }
    AnyReturnValue RawGetCompat(const TypeId& id, void* data, size_t size) const override
    {
        if (data) {
            for (const auto& c : info_->compat) {
                if (c.uid == id && c.size == size) {
                    const uint64_t v = Internal::EnumExtend(bits_, info_->any.size, info_->isUnsigned);
                    BASE_NS::CloneData(data, size, &v, size);  // low bytes == static_cast to the compat type
                    return AnyReturn::SUCCESS;
                }
            }
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue RawSetCompat(const TypeId& id, const void* data, size_t size) override
    {
        if (data) {
            for (const auto& c : info_->compat) {
                if (c.uid == id && c.size == size) {
                    uint64_t v = 0;
                    BASE_NS::CloneData(&v, sizeof(v), data, size);
                    return SetBits(Internal::EnumExtend(v, c.size, c.isUnsigned));
                }
            }
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue RawCopyFrom(const IAny& any) override
    {
        if (META_NS::IsCompatible(any, info_->any.typeId, CompatibilityDirection::GET)) {
            uint64_t v = 0;
            if (any.GetData(info_->any.typeId, &v, info_->any.size)) {
                return SetBits(v);
            }
        }
        for (const auto& c : info_->compat) {
            if (META_NS::IsCompatible(any, c.uid, CompatibilityDirection::GET)) {
                uint64_t v = 0;
                if (any.GetData(c.uid, &v, c.size)) {
                    return SetBits(Internal::EnumExtend(v, c.size, c.isUnsigned));
                }
            }
        }
        return AnyReturn::FAIL;
    }
    AnyReturnValue RawReset() override
    {
        return SetBits(0);
    }
    IAny::Ptr RawClone(const AnyCloneOptions& options) const override
    {
        if (options.role == TypeIdRole::ARRAY) {
            return info_->makeArray(prop_);
        }
        return IAny::Ptr(new CoreEnumAnyImpl(info_, prop_, options.value == CloneValueType::COPY_VALUE ? bits_ : 0));
    }

private:
    uint64_t Mask() const
    {
        return Internal::EnumBitMask(info_->any.size);
    }
    size_t IndexOfBits(uint64_t bits) const
    {
        const uint64_t m = Mask();
        bits &= m;
        for (size_t i = 0; i != prop_.metaData.enumMetaData.size(); ++i) {
            if ((uint64_t(prop_.metaData.enumMetaData[i].value) & m) == bits) {
                return i;
            }
        }
        return size_t(-1);
    }
    // Truncate to the underlying size, validate single-value membership, store. Mirrors the per-type
    // InternalSetValue: SINGLE_VALUE rejects non-member values; equal values are NOTHING_TO_DO.
    AnyReturnValue SetBits(uint64_t v)
    {
        v &= Mask();
        if (type_ == META_NS::EnumType::SINGLE_VALUE && IndexOfBits(v) == size_t(-1)) {
            return AnyReturn::INVALID_ARGUMENT;
        }
        if (v != bits_) {
            bits_ = v;
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::NOTHING_TO_DO;
    }

    const CoreEnumAnyInfo* info_;
    const CORE_NS::Property prop_;
    const META_NS::EnumType type_;
    uint64_t bits_;
};

/// Core enum array any: the shared byte-buffer storage (Internal::ByteArrayAnyImpl) + an item/array Clone
/// that carries the CORE_NS::Property so cloned items keep their live IEnum/IEngineType metadata.
class ArrayCoreEnumAnyImpl : public Internal::ByteArrayAnyImpl {
public:
    ArrayCoreEnumAnyImpl(
        const ArrayCoreEnumAnyInfo* info, const CORE_NS::Property& p, BASE_NS::vector<uint8_t> bytes = {})
        : Internal::ByteArrayAnyImpl(
              &info->array, info->bytesToVector, info->vectorToBytes, info->anyToBytes, BASE_NS::move(bytes)),
          info_(info),
          prop_(p)
    {}

protected:
    IAny::Ptr RawClone(const AnyCloneOptions& options) const override
    {
        if (options.role == TypeIdRole::ITEM) {
            return IAny::Ptr(new CoreEnumAnyImpl(info_->scalar, prop_));
        }
        return IAny::Ptr(new ArrayCoreEnumAnyImpl(
            info_, prop_, options.value == CloneValueType::COPY_VALUE ? Bytes() : BASE_NS::vector<uint8_t>{}));
    }

private:
    const ArrayCoreEnumAnyInfo* info_;
    const CORE_NS::Property prop_;
};

/**
 * @brief Thin per-enum facade so CoreEnumAny<EnumT,...> stays a concrete constructible type (sole user is
 *        ConstructCoreAny) and registration keeps the per-enum class id / name. Adds no virtuals, so its
 *        vtable is identical to CoreEnumAnyImpl's and folds to a single copy; the only per-enum residue is
 *        the static descriptor + the compatible-types/compat tables.
 */
template<typename Type, typename Conv, typename... CoreType>
class CoreEnumAny : public CoreEnumAnyImpl {
    // The shared impl stores the value as raw integer bits and does integer bit-ops for the IEnum surface,
    // so a non-enum (e.g. float) access Type would silently produce garbage. Enforce the enum invariant.
    static_assert(META_NS::is_enum_v<Type>, "CoreEnumAny requires an enum access Type");

    static IAny::Ptr MakeArray(const CORE_NS::Property& p);  // defined after ArrayCoreEnumAny; used by INFO below

public:
    using RealType = RealType_t<Type>;
    static constexpr TypeId TYPE_ID = UidFromType<Type>();

    static constexpr ObjectId StaticGetClassId()
    {
        return MakeUid<Type>(MULTI_TYPE_ANY_TAG);
    }
    static constexpr BASE_NS::string_view StaticGetTypeIdString()
    {
        return MetaType<Type>::name;
    }
    static const BASE_NS::array_view<const TypeId> StaticGetCompatibleTypes(CompatibilityDirection)
    {
        static constexpr TypeId ids[] = {TYPE_ID, UidFromType<CoreType>()..., UidFromType<int64_t>()};
        return ids;
    }

    explicit CoreEnumAny(const CORE_NS::Property& p, const Type& v = {}) : CoreEnumAnyImpl(&INFO, p, ToBits(v))
    {}

    // Keep IAny::GetTypeId(TypeIdRole) visible: this no-arg fast path would
    // otherwise hide the virtual overload (-Werror=overloaded-virtual).
    using CoreEnumAnyImpl::GetTypeId;
    TypeId GetTypeId() const
    {
        return TYPE_ID;
    }

    static uint64_t ToBits(const Type& v)
    {
        uint64_t b = 0;
        BASE_NS::CloneData(&b, sizeof(b), &v, sizeof(Type));
        return b;
    }

    static constexpr EnumCompatEntry COMPAT[]{
        EnumCompatEntry{UidFromType<CoreType>(), sizeof(CoreType), BASE_NS::is_unsigned_v<RealType_t<CoreType>>}...,
        EnumCompatEntry{UidFromType<int64_t>(), sizeof(int64_t), false},
    };
    static constexpr CoreEnumAnyInfo INFO{
        AnyTypeInfo{
            StaticGetClassId(),
            TYPE_ID,
            ArrayUidFromType<Type>(),
            ItemUidFromType<Type>(),
            StaticGetTypeIdString(),
            sizeof(Type), /*NOLINT(bugprone-sizeof-expression)*/
            &StaticGetCompatibleTypes,
        },
        BASE_NS::is_unsigned_v<RealType>,
        COMPAT,
        &MakeArray,
    };
};

/// Thin per-enum facade for the array any (see CoreEnumAny for the scalar rationale).
template<typename Type, typename Conv, typename... CoreType>
class ArrayCoreEnumAny : public ArrayCoreEnumAnyImpl {
public:
    using ArrayType = BASE_NS::vector<Type>;
    static constexpr TypeId ARRAY_TYPE_ID = ArrayUidFromType<Type>();
    static constexpr TypeId VECTOR_TYPE_ID = UidFromType<BASE_NS::vector<Type>>();

    static constexpr ObjectId StaticGetClassId()
    {
        return MakeUid<Type>(MULTI_TYPE_ARRAY_ANY_TAG);
    }
    static constexpr BASE_NS::string_view StaticGetTypeIdString()
    {
        return MetaType<ArrayType>::name;
    }
    static const BASE_NS::array_view<const TypeId> StaticGetCompatibleTypes(CompatibilityDirection)
    {
        static constexpr TypeId ids[] = {ARRAY_TYPE_ID, VECTOR_TYPE_ID};
        return ids;
    }
    static const BASE_NS::array_view<const TypeId> StaticGetItemCompatibleTypes(CompatibilityDirection dir)
    {
        return CoreEnumAny<Type, Conv, CoreType...>::StaticGetCompatibleTypes(dir);
    }

    explicit ArrayCoreEnumAny(const CORE_NS::Property& p, ArrayType v = {}) : ArrayCoreEnumAnyImpl(&INFO, p, ToBytes(v))
    {}

    // Keep IAny::GetTypeId(TypeIdRole) visible: this no-arg fast path would
    // otherwise hide the virtual overload (-Werror=overloaded-virtual).
    using ArrayCoreEnumAnyImpl::GetTypeId;
    TypeId GetTypeId() const
    {
        return ARRAY_TYPE_ID;
    }

private:
    static void BytesToVector(const uint8_t* data, size_t count, void* outVec)
    {
        auto* v = static_cast<ArrayType*>(outVec);
        v->resize(count);
        if (count) {
            BASE_NS::CloneData(v->data(), count * sizeof(Type), data, count * sizeof(Type));
        }
    }
    static void VectorToBytes(const void* inVec, BASE_NS::vector<uint8_t>& out)
    {
        const auto& v = *static_cast<const ArrayType*>(inVec);
        out.resize(v.size() * sizeof(Type));
        if (!v.empty()) {
            BASE_NS::CloneData(out.data(), out.size(), v.data(), v.size() * sizeof(Type));
        }
    }
    static bool AnyToBytes(const IAny& any, BASE_NS::vector<uint8_t>& out)
    {
        ArrayType v;
        if (any.GetValue(v)) {
            VectorToBytes(&v, out);
            return true;
        }
        return false;
    }
    static BASE_NS::vector<uint8_t> ToBytes(const ArrayType& v)
    {
        BASE_NS::vector<uint8_t> out;
        VectorToBytes(&v, out);
        return out;
    }

    static constexpr ArrayCoreEnumAnyInfo INFO{
        ArrayAnyTypeInfo{
            StaticGetClassId(),
            ARRAY_TYPE_ID,
            ItemUidFromType<Type>(),
            VECTOR_TYPE_ID,
            StaticGetTypeIdString(),
            sizeof(ArrayType),
            sizeof(Type), /*NOLINT(bugprone-sizeof-expression)*/
            &StaticGetCompatibleTypes,
            &StaticGetItemCompatibleTypes,
        },
        &CoreEnumAny<Type, Conv, CoreType...>::INFO,
        &BytesToVector,
        &VectorToBytes,
        &AnyToBytes,
    };
};

template<typename Type, typename Conv, typename... CoreType>
IAny::Ptr CoreEnumAny<Type, Conv, CoreType...>::MakeArray(const CORE_NS::Property& p)
{
    return IAny::Ptr(new ArrayCoreEnumAny<Type, Conv, CoreType...>(p));
}

META_END_NAMESPACE()

#endif
