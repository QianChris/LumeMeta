#include <benchmark/benchmark.h>
#include <deque>

#include <meta/api/object.h>
#include <meta/base/namespace.h>

#include "property_utils.h"

META_BEGIN_NAMESPACE()
namespace benchmarks {

void MetadataGetPropertyByName(benchmark::State& state)
{
    auto& objr = GetObjectRegistry();
    auto m = objr.Create<IMetadata>(ClassId::Object);
    for (int i = 0; i != 100; ++i) {
        auto name = "Thisispropertyname_" + std::to_string(i);
        m->AddProperty(ConstructProperty<float>(name.c_str()));
    }
    for (auto _ : state) {
        auto p = m->GetProperty("Thisispropertyname_50");
        if (!p) {
            state.SkipWithError("Could not find property");
            break;
        }
    }
}

void ConstructEmptyMetadata(benchmark::State& state)
{
    auto& objr = GetObjectRegistry();
    for (auto _ : state) {
        state.PauseTiming();
        BASE_NS::vector<IMetadata::Ptr> datas;
        datas.reserve(100);
        state.ResumeTiming();
        datas.push_back(objr.Create<IMetadata>(ClassId::Object));
    }
}

BENCHMARK(MetadataGetPropertyByName);
BENCHMARK(ConstructEmptyMetadata);

} // namespace benchmarks
META_END_NAMESPACE()
