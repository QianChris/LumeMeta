
#ifndef META_INTERFACE_RESOURCE_IRESOURCE_H
#define META_INTERFACE_RESOURCE_IRESOURCE_H

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Tag interface for resources
 */
class IResource : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IResource, "1d2aa2c6-7a08-49bf-a360-aecf201a8681")
public:
};

META_INTERFACE_TYPE(META_NS::IResource)

META_END_NAMESPACE()

#endif
