/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: Resource container
 * Author: Mikael Kilpeläinen
 * Create: 2024-11-07
 */
#ifndef META_SRC_RESOURCE_CONTAINER_H
#define META_SRC_RESOURCE_CONTAINER_H

#include <meta/interface/resource/intf_resource_container.h>

#include "../container/object_flat_container.h"

META_BEGIN_NAMESPACE()

class ResourceContainer : public IntroduceInterfaces<ObjectFlatContainer, IResourceContainer> {
    META_OBJECT(ResourceContainer, ClassId::ResourceContainer, IntroduceInterfaces)
};

META_END_NAMESPACE()

#endif
