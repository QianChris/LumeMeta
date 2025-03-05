/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 * Description: Instantiating object provider wrapper
 * Author: Mikael Kilpeläinen
 */
#ifndef META_API_MODEL_INSTANTIATING_OBJECT_PROVIDER_H
#define META_API_MODEL_INSTANTIATING_OBJECT_PROVIDER_H

#include <meta/api/internal/object_api.h>
#include <meta/interface/model/intf_object_provider.h>

META_BEGIN_NAMESPACE()

/*
 * @brief Wrapper for META_NS::ClassId::InstantiatingObjectProvider which is implementation of object provider
 * that will instantiate objects when needed.
 */
class InstantiatingObjectProvider final
    : public Internal::ObjectInterfaceAPI<InstantiatingObjectProvider, ClassId::InstantiatingObjectProvider> {
    META_API(InstantiatingObjectProvider)
    META_API_OBJECT_CONVERTIBLE(IObjectProvider)

    META_API_CACHE_INTERFACE(IModelObjectProvider, Provider)
    META_API_CACHE_INTERFACE(IInstantiatingObjectProvider, InstProvider)

public:
    META_API_INTERFACE_PROPERTY_CACHED(Provider, CacheHint, size_t)

    /** See IInstantiatingObjectProvider::SetObjectClassUid */
    InstantiatingObjectProvider& ObjectClassUid(const ObjectId& id)
    {
        META_API_CALL_INTERFACE(InstProvider, SetObjectClassId(id));
        return *this;
    }

    /** See IModelObjectProvider::SetDataModel */
    InstantiatingObjectProvider& DataModel(const IDataModel::Ptr& model)
    {
        META_API_CALL_INTERFACE(Provider, SetDataModel(model));
        return *this;
    }

    /** See IModelObjectProvider::GetDataModel */
    IDataModel::Ptr GetDataModel() const
    {
        return META_API_CALL_INTERFACE(Provider, GetDataModel());
    }
};

META_END_NAMESPACE()

#endif
