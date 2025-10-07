
#include "refuri_builder.h"

#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/serialization/intf_exporter_state.h>

META_BEGIN_NAMESPACE()
namespace Serialization {

IObject::Ptr RefUriBuilder::ResolveUriSegment(
    const IObject::ConstPtr& ptr, RefUri& uri, const IObject::ConstPtr& previous) const
{
    if (auto property = interface_cast<IProperty>(ptr)) {
        uint32_t index = -1;
        if (ArrayPropertyLock lock { property }) {
            for (size_t i = 0; i != lock->GetSize(); ++i) {
                if (interface_pointer_cast<IObject>(GetConstPointer(lock->GetAnyAt(i))) == previous) {
                    index = i;
                    break;
                }
            }
        }
        uri.PushPropertySegment(property->GetName(), index);
        auto owner = interface_pointer_cast<IObject>(property->GetOwner());
        if (!owner) {
            CORE_LOG_E("No Owner for property '%s' when exporting weak ptr", property->GetName().c_str());
        }
        return owner;
    }
    if (auto instance = interface_cast<IObjectInstance>(ptr)) {
        auto parent = ptr->Resolve(RefUri::ParentUri());

        if (auto i = interface_cast<META_NS::IProperty>(parent)) {
            return parent;
        }

        if (auto i = interface_cast<IAttach>(parent)) {
            if (auto cont = i->GetAttachmentContainer(false)) {
                if (ContainsObject(cont, ptr)) {
                    uri.PushAttachmentSegment(instance->GetName());
                    return parent;
                }
            }
        }
        if (auto context = interface_cast<IObjectContext>(ptr)) {
            uri.PushObjectContextSegment();
        } else {
            uri.PushObjectSegment(instance->GetName());
        }
        return parent;
    }
    return nullptr;
}

bool RefUriBuilder::IsAnchorType(const ObjectId& id) const
{
    for (auto&& v : anchorTypes_) {
        if (v == id) {
            return true;
        }
    }
    return false;
}

RefUri RefUriBuilder::BuildRefUri(IExporterState& state, const IObject::ConstPtr& object)
{
    auto userContext = state.GetUserContext();
    auto ops = state.GetOptions();
    IObject::ConstPtr p = object;
    auto original = p;
    IObject::ConstPtr previous;
    RefUri uri;
    while (p) {
        if (auto obj = interface_cast<IObjectInstance>(p)) {
            auto iid = state.ConvertInstanceId(obj->GetInstanceId());
            if (userContext && ops.useRelativeRefuriIfContextSet) {
                if (userContext.get() == obj) {
                    uri.SetStartsFromRoot(true);
                    return uri;
                }
            }
            if (IsAnchorType(obj->GetClassId())) {
                uri.SetStartsFromRoot(true);
                return uri;
            }
            if (state.HasBeenExported(iid) || state.IsGlobalObject(obj->GetInstanceId())) {
                uri.SetBaseObjectUid(iid.ToUid());
                return uri;
            }
        }
        auto newp = ResolveUriSegment(p, uri, previous);
        previous = p;
        p = newp;
    }
    return {};
}

} // namespace Serialization
META_END_NAMESPACE()
