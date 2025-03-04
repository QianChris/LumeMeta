/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: ObjectDataModel user API
 * Author: Slawomir Grabowski
 * Create: 2022-11-25
 */

#ifndef META_API_MODEL_CONTAINER_DATA_MODEL_H
#define META_API_MODEL_CONTAINER_DATA_MODEL_H

#include <meta/api/internal/object_api.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/model/intf_data_model.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Wrapper for META_NS::ClassId::ContainerDataModel which provides data model interface for container.
 */
class ContainerDataModel : public Internal::ObjectInterfaceAPI<ContainerDataModel, ClassId::ContainerDataModel> {
    META_API(ContainerDataModel)

    META_API_OBJECT_CONVERTIBLE(IDataModel)
    META_API_OBJECT_CONVERTIBLE(IContainer)

    META_API_CACHE_INTERFACE(IDataModel, DataModel)
    META_API_CACHE_INTERFACE(IContainer, Container)

public:
    /** See IContainer::Add */
    ContainerDataModel& Record(IMetadata::Ptr record)
    {
        META_API_CALL_INTERFACE(Container, Add(BASE_NS::move(record)));
        return *this;
    }

    /** See IContainer::Remove */
    ContainerDataModel& Remove(size_t index)
    {
        META_API_CALL_INTERFACE(Container, Remove(index));
        return *this;
    }

    /** See IContainer::Insert */
    ContainerDataModel& Insert(size_t index, IMetadata::Ptr record)
    {
        META_API_CALL_INTERFACE(Container, Insert(index, BASE_NS::move(record)));
        return *this;
    }
};

META_END_NAMESPACE()

#endif
