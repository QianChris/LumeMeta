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
#ifndef META_EXT_OBJECT_FACTORY_H
#define META_EXT_OBJECT_FACTORY_H

#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_object_factory.h>
#include <meta/interface/intf_object_registry.h>

#if defined(GetClassInfo)
#undef GetClassInfo
#endif

META_BEGIN_NAMESPACE()

/**
 * @brief A base class which can be used to implement custom object factories.
 */
class ObjectFactory : public IObjectFactory {
    // Ignore refcounts as DefaultObjectFactory is always created as static object
    // for the class for which it implements a factory.
    void Ref() override
    {}
    void Unref() override
    {}

public:
    ObjectFactory(const META_NS::ClassInfo& info) : info_(info)
    {}
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override
    {
        if (uid == CORE_NS::IInterface::UID) {
            return this;
        }
        if (uid == IClassInfo::UID) {
            return this;
        }
        if (uid == IObjectFactory::UID) {
            return this;
        }
        return nullptr;
    }
    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override
    {
        return const_cast<ObjectFactory*>(this)->GetInterface(uid);
    }
    const META_NS::ClassInfo& GetClassInfo() const override
    {
        return info_;
    }

private:
    META_NS::ClassInfo info_;
};

template<typename FinalClass>
class PlainObjectFactory : public ObjectFactory {
public:
    PlainObjectFactory(const META_NS::ClassInfo& info) : ObjectFactory(info)
    {}
    IObject::Ptr CreateInstance() const override
    {
        return IObject::Ptr{new FinalClass};
    }
    ClassConstructionType ConstructionType() const override
    {
        return ClassConstructionType::SIMPLE;
    }
};

/**
 * @brief The DefaultObjectFactory class implements the default object factory used
 *        by ObjectFwd and such. hash
 */
template<typename FinalClass>
class DefaultObjectFactory final : public PlainObjectFactory<FinalClass> {
public:
    DefaultObjectFactory(const META_NS::ClassInfo& info) : PlainObjectFactory<FinalClass>(info)
    {}
    const StaticObjectMetadata* GetClassStaticMetadata() const override
    {
        return FinalClass::StaticMetadata();
    }
    const BASE_NS::vector<BASE_NS::Uid>& GetClassInterfaces() const override
    {
        static auto interfaces = FinalClass::GetInterfacesVector();
        return interfaces;
    }
    ClassConstructionType ConstructionType() const override
    {
        return ClassConstructionType::BUILD;
    }
};

/**
 * @brief Non-templated object factory whose per-type behaviour — construction, static metadata, and
 *        implemented interfaces — is supplied as function pointers at construction time.
 *
 * This is the factory META_OBJECT uses (via META_DEFINE_OBJECT_TYPE_INFO) to register object types.
 * Prefer it over DefaultObjectFactory<T>: because it is not a template, one factory class serves
 * every registered type, instead of instantiating a separate factory class — with its own vtable and
 * accessor methods — per registered type. DefaultObjectFactory<T> is retained for callers that want a
 * typed factory directly; the two are behaviourally equivalent.
 */
class GenericObjectFactory final : public ObjectFactory {
public:
    using CreateFunc = IObject::Ptr (*)();
    using MetadataFunc = const StaticObjectMetadata* (*)();
    using InterfacesFunc = BASE_NS::vector<BASE_NS::Uid> (*)();

    GenericObjectFactory(const META_NS::ClassInfo& info, CreateFunc create, MetadataFunc metadata,
        InterfacesFunc interfaces, ClassConstructionType constructionType)
        : ObjectFactory(info),
          create_(create),
          metadata_(metadata),
          interfaces_(interfaces()),
          constructionType_(constructionType)
    {}
    IObject::Ptr CreateInstance() const override
    {
        return create_();
    }
    const StaticObjectMetadata* GetClassStaticMetadata() const override
    {
        return metadata_();
    }
    const BASE_NS::vector<BASE_NS::Uid>& GetClassInterfaces() const override
    {
        return interfaces_;
    }
    ClassConstructionType ConstructionType() const override
    {
        return constructionType_;
    }

private:
    CreateFunc create_;
    MetadataFunc metadata_;
    BASE_NS::vector<BASE_NS::Uid> interfaces_;
    ClassConstructionType constructionType_;
};

/**
 *  Helper macro emitting GetFactory()/OBJECT_INFO so a class can be registered with the object
 *  registry. Constructs a shared GenericObjectFactory configured with the type's
 *  create/metadata/interfaces functions, so no per-type factory class is instantiated.
 */
#define META_DEFINE_OBJECT_TYPE_INFO(FinalClass, ClassInfo)                                                 \
    /* Keep DefaultObjectFactory<T> usable on this class (it constructs via new T directly). */             \
    friend class META_NS::DefaultObjectFactory<FinalClass>;                                                 \
                                                                                                            \
public:                                                                                                     \
    static META_NS::IObjectFactory::Ptr GetFactory()                                                        \
    {                                                                                                       \
        static META_NS::GenericObjectFactory factory(                                                       \
            ClassInfo,                                                                                      \
            +[]() -> META_NS::IObject::Ptr { return META_NS::IObject::Ptr{new FinalClass}; },               \
            &FinalClass::StaticMetadata,                                                                    \
            &FinalClass::GetInterfacesVector,                                                               \
            META_NS::ClassConstructionType::BUILD);                                                         \
        return META_NS::IObjectFactory::Ptr{&factory};                                                      \
    }                                                                                                       \
    static constexpr const META_NS::ObjectTypeInfo OBJECT_INFO{{META_NS::ObjectTypeInfo::UID}, GetFactory}; \
                                                                                                            \
private:

META_END_NAMESPACE()

#endif
