/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Container observer API
 * Author: Lauri Jaaskela
 * Create: 2023-03-21
 */

#ifndef META_API_CONTAINER_OBSERVER_H
#define META_API_CONTAINER_OBSERVER_H

#include <meta/api/internal/object_api.h>
#include <meta/api/make_callback.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_container_observer.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The ContainerObserver class provides a wrapper for META_NS::ClassId::ContainerObserver.
 */
class ContainerObserver final
    : public Internal::ObjectInterfaceAPI<ContainerObserver, META_NS::ClassId::ContainerObserver> {
    META_API(ContainerObserver)
    META_API_CACHE_INTERFACE(META_NS::IContainerObserver, Observer)
public:
    /**
     * @brief See IContainerObserver::SetContainer.
     */
    auto& Container(const IContainer::Ptr& container)
    {
        META_API_CALL_INTERFACE(Observer, SetContainer(container));
        return *this;
    }
    /**
     * @brief See IContainerObserver::OnDescendantAdded.
     */
    template<class Callback>
    auto& OnDescendantChanged(Callback&& callback)
    {
        if (auto p = META_API_CALL_INTERFACE(Observer, OnDescendantChanged())) {
            p->AddHandler(META_NS::MakeCallback<META_NS::IOnChildChanged>(BASE_NS::forward<Callback>(callback)));
        }
        return *this;
    }
};

META_END_NAMESPACE()

#endif // META_API_CONTAINER_OBSERVER_H
