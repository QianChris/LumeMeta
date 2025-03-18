/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: IContent user API
 * Author: Mikael Kilpeläinen
 * Create: 2023-09-06
 */

#ifndef META_API_CONTENT_OBJECT_H
#define META_API_CONTENT_OBJECT_H

#include <meta/api/property/binding.h>
#include <meta/interface/intf_content.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_required_interfaces.h>

#include "object.h"

META_BEGIN_NAMESPACE()

class ContentObject final : public Internal::ObjectInterfaceAPI<ContentObject, ClassId::ContentObject> {
    META_API(ContentObject)
    META_API_OBJECT_CONVERTIBLE(IContent)
    META_API_CACHE_INTERFACE(IContent, Content)
    META_API_OBJECT_CONVERTIBLE(IRequiredInterfaces)
    META_API_CACHE_INTERFACE(IRequiredInterfaces, RequiredInterfaces)
public:
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Content, Content, IObject::Ptr)
    META_API_INTERFACE_PROPERTY_CACHED(Content, ContentSearchable, bool)

    ContentObject& SerializeContent(bool serialize)
    {
        SetObjectFlags(ObjectRef(), ObjectFlagBits::SERIALIZE_HIERARCHY, serialize);
        return *this;
    }
    bool SetContent(const IObject::Ptr& content)
    {
        return META_API_CALL_INTERFACE(Content, SetContent(content));
    }
    bool SetRequiredInterfaces(const BASE_NS::vector<TypeId>& interfaces)
    {
        return META_API_CALL_INTERFACE(RequiredInterfaces, SetRequiredInterfaces(interfaces));
    }
    BASE_NS::vector<TypeId> GetRequiredInterfaces() const
    {
        return META_API_CALL_INTERFACE(RequiredInterfaces, GetRequiredInterfaces());
    }
};

META_END_NAMESPACE()

#endif
