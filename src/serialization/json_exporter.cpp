
#include "json_exporter.h"

#include <meta/api/util.h>

#include "backend/json_output.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

ReturnError JsonExporter::Export(CORE_NS::IFile& output, const IObject::ConstPtr& object)
{
    auto tree = Export(object);
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
ISerNode::Ptr JsonExporter::Export(const IObject::ConstPtr& object)
{
    return exp_.Export(object);
}
void JsonExporter::SetInstanceIdMapping(BASE_NS::unordered_map<InstanceId, InstanceId> map)
{
    exp_.SetInstanceIdMapping(BASE_NS::move(map));
}

} // namespace Serialization
META_END_NAMESPACE()
