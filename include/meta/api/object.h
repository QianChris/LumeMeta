/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2025. All rights reserved.
 * Description: Definition of base object interface wrapper objects.
 * Author: Lauri Jaaskela
 * Create: 2025-03-20
 */

#ifndef META_API_OBJECT_H
#define META_API_OBJECT_H

#include <meta/api/interface_object.h>
#include <meta/api/make_callback.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_content.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_required_interfaces.h>
#include <meta/interface/property/construct_property.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Creates an instance of a class with given class id.
 * @param clsi The class info of the class to instantiate.
 * @return The created object.
 */
inline IObject::Ptr CreateInstance(const META_NS::ClassInfo& clsi)
{
    return META_NS::GetObjectRegistry().Create(clsi);
}

/**
 * @brief Wrapper class for for objects which implement IObject.
 * @note Object can be initialized with Any object created through IObjectRegistry::Create().
 */
class Object : public InterfaceObject<IObject> {
public:
    META_INTERFACE_OBJECT(Object, InterfaceObject<IObject>, IObject)
    META_INTERFACE_OBJECT_INSTANTIATE(Object, ClassId::Object)

    auto GetClassId() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetClassId());
    }
    auto GetClassName() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetClassName());
    }
    auto GetName() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetName());
    }
    auto GetInterfaces() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetInterfaces());
    }
    auto Resolve(const RefUri& uri) const
    {
        return Object(META_INTERFACE_OBJECT_CALL_PTR(Resolve(uri)));
    }
    auto GetSelf() const
    {
        return Object(META_INTERFACE_OBJECT_CALL_PTR(GetSelf()));
    }
};

/**
 * @brief Wrapper class for objects which implement IMetadata.
 */
class Metadata : public InterfaceObject<IMetadata> {
public:
    META_INTERFACE_OBJECT(Metadata, InterfaceObject<IMetadata>, IMetadata)

    auto GetProperties()
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetProperties());
    }
    auto GetProperties() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetProperties());
    }
    auto GetFunctions()
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetFunctions());
    }
    auto GetFunctions() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetFunctions());
    }
    auto GetEvents()
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetEvents());
    }
    auto GetEvents() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetEvents());
    }
    auto GetAllMetadatas(MetadataType types) const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetAllMetadatas(types));
    }
    auto GetMetadata(MetadataType type, BASE_NS::string_view name) const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetMetadata(type, name));
    }
    auto GetProperty(BASE_NS::string_view name, MetadataQuery query)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetProperty(name, query));
    }
    auto GetProperty(BASE_NS::string_view name)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetProperty(name));
    }
    auto GetProperty(BASE_NS::string_view name, MetadataQuery query) const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetProperty(name, query));
    }
    auto GetProperty(BASE_NS::string_view name) const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetProperty(name));
    }
    auto GetEvent(BASE_NS::string_view name)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetEvent(name));
    }
    auto GetFunction(BASE_NS::string_view name)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetFunction(name));
    }
    auto AddProperty(const IProperty::Ptr& property)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(AddProperty(property));
    }
    auto RemoveProperty(const IProperty::Ptr& property)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(RemoveProperty(property));
    }
    template<class T>
    auto AddProperty(BASE_NS::string_view name, const T& value = {}, ObjectFlagBitsValue flags = ObjectFlagBits::NONE)
    {
        if (!GetProperty(name)) {
            return AddProperty(ConstructProperty<T>(name, value, flags));
        }
        return false;
    }
    template<class T>
    auto GetProperty(BASE_NS::string_view name) const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(template GetProperty<T>(name));
    }
};

/**
 * @brief Wrapper class for objects which implement IContainer.
 */
class Container : public InterfaceObject<IContainer> {
public:
    META_INTERFACE_OBJECT(Container, InterfaceObject<IContainer>, IContainer)
    META_INTERFACE_OBJECT_INSTANTIATE(Container, ClassId::ObjectContainer)
    auto GetAll() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetAll());
    }
    auto GetAt(IContainer::SizeType index)
    {
        return Object(META_INTERFACE_OBJECT_CALL_PTR(GetAt(index)));
    }
    auto GetSize() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetSize());
    }
    auto FindAll(const IContainer::FindOptions& options) const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(FindAll(options));
    }
    auto FindAny(const IContainer::FindOptions& options) const
    {
        return Object(META_INTERFACE_OBJECT_CALL_PTR(FindAny(options)));
    }
    auto FindByName(BASE_NS::string_view name) const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(FindByName(name));
    }
    auto Add(const IObject::Ptr& object)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(Add(object));
    }
    auto Insert(IContainer::SizeType index, const IObject::Ptr& object)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(Insert(index, object));
    }
    auto Remove(IContainer::SizeType index)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(Remove(index));
    }
    auto Remove(const IObject::Ptr& child)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(Add(child));
    }
    auto Move(IContainer::SizeType fromIndex, IContainer::SizeType toIndex)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(Move(fromIndex, toIndex));
    }
    auto Move(const IObject::Ptr& child, IContainer::SizeType toIndex)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(Move(child, toIndex));
    }
    auto Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(Replace(child, replaceWith));
    }
    auto Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith, bool addAlways)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(Replace(child, replaceWith, addAlways));
    }
    auto RemoveAll()
    {
        META_INTERFACE_OBJECT_CALL_PTR(RemoveAll());
    }
};

/**
 * @brief Wrapper class for objects which implement INamed.
 */
class Named : public Object {
public:
    META_INTERFACE_OBJECT(Named, Object, INamed)
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::string, Name)
};

/**
 * @brief Wrapper class for objects which implement IAttach.
 */
class AttachmentContainer : public InterfaceObject<IAttach> {
public:
    META_INTERFACE_OBJECT(AttachmentContainer, InterfaceObject<IAttach>, IAttach)
    template<class T, class U>
    auto Attach(const T& object, const U& dataContext)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(
            Attach(interface_pointer_cast<IObject>(object), interface_pointer_cast<IObject>(dataContext)));
    }
    template<class T>
    auto Attach(const T& object)
    {
        return Attach(interface_pointer_cast<IObject>(object), META_NS::IObject::Ptr {});
    }
    template<class T>
    auto Detach(const T& object)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(Detach(interface_pointer_cast<IObject>(object)));
    }
    bool HasAttachments() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(HasAttachments());
    }
    auto GetAttachments(BASE_NS::array_view<const TypeId> uids, bool strict = false) const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(
            GetAttachments(BASE_NS::vector<TypeId>(uids.begin(), uids.end()), strict));
    }
    auto GetAttachments() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetAttachments());
    }
    template<class T>
    BASE_NS::vector<typename T::Ptr> GetAttachments() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(template GetAttachments<T>());
    }
};

/**
 * @brief Wrapper class for objects which implement IContent.
 */
class ContentObject : public Object {
public:
    META_INTERFACE_OBJECT(ContentObject, Object, IContent)

    META_INTERFACE_OBJECT_READONLY_PROPERTY(IObject::Ptr, Content)
    META_INTERFACE_OBJECT_PROPERTY(bool, ContentSearchable)
    auto SetContent(const IObject::Ptr& content) const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(SetContent(content));
    }
    auto& SerializeContent(bool serialize)
    {
        auto object = GetPtr<IObject>();
        META_NS::SetObjectFlags(object, ObjectFlagBits::SERIALIZE_HIERARCHY, serialize);
        return *this;
    }
};

/**
 * @brief Wrapper class for objects which implement IRequiredInterfaces.
 */
class RequiredInterfaces : public InterfaceObject<IRequiredInterfaces> {
public:
    META_INTERFACE_OBJECT(RequiredInterfaces, InterfaceObject<IRequiredInterfaces>, IRequiredInterfaces)

    auto SetRequiredInterfaces(const BASE_NS::vector<TypeId>& interfaces)
    {
        return META_INTERFACE_OBJECT_CALL_PTR(SetRequiredInterfaces(interfaces));
    }
    auto GetRequiredInterfaces() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetRequiredInterfaces());
    }
};

/**
 * @brief Creates an instance of a class with given class id and converts it to a given interface type.
 * @param clsi The class info of the class to instantiate.
 * @return The created object converted to given interface type.
 */
template<typename Interface>
inline auto CreateInstance(const META_NS::ClassInfo& clsi)
{
    return interface_pointer_cast<Interface>(CreateInstance(clsi));
}
template<typename Interface>
auto CreateInstance() = delete;
/// Returns a default object which implements IObject
template<>
inline auto CreateInstance<IObject>()
{
    return Object(CreateNew);
}
/// Returns a default object which implements IMetadata
template<>
inline auto CreateInstance<IMetadata>()
{
    return Metadata(CreateInstance(META_NS::ClassId::Object));
}
/// Returns a default object which implements IAttach
template<>
inline auto CreateInstance<IAttach>()
{
    return AttachmentContainer(CreateInstance(META_NS::ClassId::Object));
}
/// Returns a default object which implements IContainer
template<>
inline auto CreateInstance<IContainer>()
{
    return Container(CreateNew);
}

META_END_NAMESPACE()

#endif // META_API_OBJECT_H
