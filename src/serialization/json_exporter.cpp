
#include "json_exporter.h"

#include <meta/api/util.h>

#include "backend/json_output.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

ReturnError JsonExporter::Export(CORE_NS::IFile& output, const IObject::ConstPtr& object, ExportOptions options)
{
    auto tree = Export(object, options);
    if (!tree) {
        return GenericError::FAIL;
    }
    JsonOutput backend;
    auto json = backend.Process(tree);
    if (json.empty()) {
        return GenericError::FAIL;
    }
    output.Write(json.c_str(), json.size());
    return GenericError::SUCCESS;
}
ISerNode::Ptr JsonExporter::Export(const IObject::ConstPtr& object, ExportOptions options)
{
    return exp_.Export(object, options);
}
void JsonExporter::SetInstanceIdMapping(BASE_NS::unordered_map<InstanceId, InstanceId> map)
{
    exp_.SetInstanceIdMapping(BASE_NS::move(map));
}
void JsonExporter::SetResourceManager(CORE_NS::IResourceManager::Ptr p)
{
    exp_.SetResourceManager(BASE_NS::move(p));
}
void JsonExporter::SetUserContext(IObject::Ptr p)
{
    exp_.SetUserContext(BASE_NS::move(p));
}
void JsonExporter::SetMetadata(SerMetadata m)
{
    exp_.SetMetadata(BASE_NS::move(m));
}

} // namespace Serialization
META_END_NAMESPACE()
