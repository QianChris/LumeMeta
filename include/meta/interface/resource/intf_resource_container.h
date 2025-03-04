
#ifndef META_INTERFACE_RESOURCE_IRESOURCE_CONTAINER_H
#define META_INTERFACE_RESOURCE_IRESOURCE_CONTAINER_H

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Tag interface for container containing resources
 */
class IResourceContainer : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IResourceContainer, "5178e760-5368-4f26-913f-1541026f91be")
public:
};

META_INTERFACE_TYPE(META_NS::IResourceContainer)

META_END_NAMESPACE()

#endif
