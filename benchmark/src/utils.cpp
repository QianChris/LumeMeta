
#include "utils.h"

#include <meta/ext/object_fwd.h>
#include <meta/interface/interface_macros.h>

META_BEGIN_NAMESPACE()
namespace benchmarks {
namespace {

class BenchmarkType : public IntroduceInterfaces<ObjectFwd, IBenchmarkType> {
    META_OBJECT(BenchmarkType, ClassId::BenchmarkType, IntroduceInterfaces)
};

} // namespace

void RegisterBenchmarkTypes()
{
    META_NS::RegisterObjectType<BenchmarkType>();

    //    RegisterPropertyType<IBenchmarkType>();
    //    RegisterPropertyType<BenchmarkEnum>();
}

void UnregisterBenchmarkTypes()
{
    //    UnRegisterPropertyType<BenchmarkEnum>();
    //    UnRegisterPropertyType<IBenchmarkType>();

    META_NS::UnregisterObjectType<BenchmarkType>();
}

IBenchmarkType::Ptr CreateBenchmarkType()
{
    return META_NS::GetObjectRegistry().Create<IBenchmarkType>(ClassId::BenchmarkType);
}

} // namespace benchmarks
META_END_NAMESPACE()
