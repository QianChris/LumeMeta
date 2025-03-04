/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 * Description: Content loader object provider wrapper
 * Author: Mikael Kilpeläinen
 */
#ifndef META_API_MODEL_CONTENT_LOADER_OBJECT_PROVIDER_H
#define META_API_MODEL_CONTENT_LOADER_OBJECT_PROVIDER_H

#include <meta/api/internal/object_api.h>
#include <meta/interface/model/intf_object_provider.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Wrapper for META_NS::ClassId::ContentLoaderObjectProvider which provides object provider interface for loaded
 * content.
 */
class ContentLoaderObjectProvider
    : public Internal::ObjectInterfaceAPI<ContentLoaderObjectProvider, ClassId::ContentLoaderObjectProvider> {
    META_API(ContentLoaderObjectProvider)
    META_API_OBJECT_CONVERTIBLE(IObjectProvider)

    META_API_CACHE_INTERFACE(IModelObjectProvider, Provider)
    META_API_CACHE_INTERFACE(IContentLoaderObjectProvider, ContentLoaderProvider)

public:
    /** See IContentLoaderObjectProvider::SetContentLoader */
    ContentLoaderObjectProvider& SetContentLoader(const IContentLoader::Ptr& loader)
    {
        META_API_CALL_INTERFACE(ContentLoaderProvider, SetContentLoader(loader));
        return *this;
    }

    /** See IModelObjectProvider::SetDataModel */
    ContentLoaderObjectProvider& DataModel(const IDataModel::Ptr& model)
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
