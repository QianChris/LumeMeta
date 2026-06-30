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

#ifndef META_EXT_ANY_BUILDER_H
#define META_EXT_ANY_BUILDER_H

#include <meta/base/meta_types.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/intf_property_register.h>

META_BEGIN_NAMESPACE()

/// The default version of any builder that is used for registering any types
template<typename AnyType>
class DefaultAnyBuilder : public AnyBuilder {
public:
    IAny::Ptr Construct() override
    {
        return IAny::Ptr(new AnyType);
    }
    ObjectId GetObjectId() const override
    {
        return AnyType::StaticGetClassId();
    }
    BASE_NS::string GetTypeName() const override
    {
        return BASE_NS::string(AnyType::StaticGetTypeIdString());
    }
};

/**
 * @brief Non-templated any builder whose per-type behaviour — construction, object id, type name — is
 *        supplied at construction time (a construct thunk plus the type's static id/name as data).
 *
 * This is what CreateAnyBuilder<T>() (and hence the RegisterAny helpers below) uses to register any
 * types. Prefer it over DefaultAnyBuilder<T>: because it is not a template, one builder class — and one
 * CreateShared instantiation — serves every registered any type, instead of a separate builder class
 * (vtable + accessors) and a separate shared-storage block per type. DefaultAnyBuilder<T> is retained
 * for direct callers; the two are behaviourally equivalent.
 */
class GenericAnyBuilder final : public AnyBuilder {
public:
    using ConstructFunc = IAny::Ptr (*)();
    GenericAnyBuilder(ConstructFunc construct, const ObjectId& id, BASE_NS::string_view typeName)
        : construct_(construct), id_(id), typeName_(typeName)
    {}
    IAny::Ptr Construct() override
    {
        return construct_();
    }
    ObjectId GetObjectId() const override
    {
        return id_;
    }
    BASE_NS::string GetTypeName() const override
    {
        return BASE_NS::string(typeName_);
    }

private:
    ConstructFunc construct_;
    ObjectId id_;
    BASE_NS::string_view typeName_;  // backed by AnyType::StaticGetTypeIdString()'s static storage
};

/// Create a shared any builder for AnyType, configured via GenericAnyBuilder (one class for all types).
template<typename AnyType>
BASE_NS::shared_ptr<AnyBuilder> CreateAnyBuilder()
{
    return CreateShared<GenericAnyBuilder>(
        +[]() -> IAny::Ptr { return IAny::Ptr(new AnyType); },
        AnyType::StaticGetClassId(),
        AnyType::StaticGetTypeIdString());
}

/// Register user defined any type
template<typename AnyType>
void RegisterUserAny()
{
    GetObjectRegistry().GetPropertyRegister().RegisterAny(CreateAnyBuilder<AnyType>());
}

/// Register user type using built-in any type
template<typename Type>
void RegisterTypeForBuiltinArrayAny();
template<typename Type>
void RegisterTypeForBuiltinAny()
{
    GetObjectRegistry().GetPropertyRegister().RegisterAny(CreateAnyBuilder<AnyType<Type>>());
    // Also register the array variant so Any/Enum Clone(role=ARRAY) can construct it via the
    // registry instead of force-instantiating the array template in the per-type clone path.
    RegisterTypeForBuiltinArrayAny<Type>();
}

/// Register user type using built-in array any type
template<typename Type>
void RegisterTypeForBuiltinArrayAny()
{
    GetObjectRegistry().GetPropertyRegister().RegisterAny(CreateAnyBuilder<ArrayAnyType<Type>>());
}

/// Unregister, counter part for RegisterUserAny
template<typename Type>
void UnregisterUserAny()
{
    GetObjectRegistry().GetPropertyRegister().UnregisterAny(Type::StaticGetClassId());
}

/// Unregister, counter part for RegisterTypeForBuiltinAny
template<typename Type>
void UnregisterTypeForBuiltinArrayAny();
template<typename Type>
void UnregisterTypeForBuiltinAny()
{
    UnregisterTypeForBuiltinArrayAny<Type>();
    GetObjectRegistry().GetPropertyRegister().UnregisterAny(AnyType<Type>::StaticGetClassId());
}

/// Unregister, counter part for RegisterTypeForBuiltinArrayAny
template<typename Type>
void UnregisterTypeForBuiltinArrayAny()
{
    GetObjectRegistry().GetPropertyRegister().UnregisterAny(ArrayAnyType<Type>::StaticGetClassId());
}

META_END_NAMESPACE()

#endif
