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
#ifndef META_INTERFACE_DETAIL_MULTI_TYPE_ANY_H
#define META_INTERFACE_DETAIL_MULTI_TYPE_ANY_H

#include <meta/interface/detail/base_any.h>

META_BEGIN_NAMESPACE()

constexpr char MULTI_TYPE_ANY_TAG[] = "MultiAny";
constexpr char MULTI_TYPE_ARRAY_ANY_TAG[] = "MultsAny";

struct StaticCastConv {
    template<typename T1, typename T2>
    static T1 ToType(const T2& v)
    {
        return static_cast<T1>(v);
    }
};

template<typename Type, typename Conv, typename... CompatType>
class MultiTypeAny : public AnyBase {
public:
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
        static constexpr TypeId ids[] = {TYPE_ID, UidFromType<CompatType>()...};
        return ids;
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
        if (!data) {
            return AnyReturn::INVALID_ARGUMENT;
        }
        AnyReturnValue res = AnyReturn::INVALID_ARGUMENT;
        [[maybe_unused]] bool r = ((res = [&]() -> AnyReturnValue {
            if (sizeof(CompatType) == size && id == UidFromType<CompatType>()) { /*NOLINT(bugprone-sizeof-expression)*/
                *static_cast<CompatType*>(data) = Conv::template ToType<CompatType>(InternalGetValue());
                return AnyReturn::SUCCESS;
            }
            return AnyReturn::INVALID_ARGUMENT;
        }()) || ...);
        return res;
    }
    AnyReturnValue RawSetCompat(const TypeId& id, const void* data, size_t size) override
    {
        if (!data) {
            return AnyReturn::INVALID_ARGUMENT;
        }
        AnyReturnValue res = AnyReturn::INVALID_ARGUMENT;
        [[maybe_unused]] bool r = ((res = [&]() -> AnyReturnValue {
            if (sizeof(CompatType) == size && id == UidFromType<CompatType>()) { /*NOLINT(bugprone-sizeof-expression)*/
                return InternalSetValue(Conv::template ToType<Type>(*static_cast<const CompatType*>(data)));
            }
            return AnyReturn::INVALID_ARGUMENT;
        }()) || ...);
        return res;
    }
    AnyReturnValue RawCopyFrom(const IAny& any) override
    {
        if (META_NS::IsCompatible(any, TYPE_ID, CompatibilityDirection::GET)) {
            Type value;
            if (any.GetValue(value)) {
                return InternalSetValue(value);
            }
        }

        AnyReturnValue res = AnyReturn::FAIL;
        [[maybe_unused]] bool r = ((res = [&]() -> AnyReturnValue {
            if (META_NS::IsCompatible(any, UidFromType<CompatType>(), CompatibilityDirection::GET)) {
                CompatType value;
                if (any.GetValue(value)) {
                    return InternalSetValue(Conv::template ToType<Type>(value));
                }
            }
            return AnyReturn::FAIL;
        }()) || ...);
        return res;
    }
    AnyReturnValue RawReset() override
    {
        return InternalSetValue(Type {});
    }

    virtual AnyReturnValue InternalSetValue(const Type& value) = 0;
    virtual const Type& InternalGetValue() const = 0;
};

// idea is just to provide array any that gives you correct any type when cloning ITEM
template<typename Type>
class ArrayMultiTypeAnyBase : public ArrayAny<Type> {
    using Super = ArrayAny<Type>;

public:
    using Super::Super;

    static constexpr ObjectId StaticGetClassId()
    {
        return MakeUid<Type>(MULTI_TYPE_ARRAY_ANY_TAG);
    }

    ObjectId GetClassId() const override
    {
        return StaticGetClassId();
    }
};

META_END_NAMESPACE()

#endif
