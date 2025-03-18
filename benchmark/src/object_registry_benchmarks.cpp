#include <benchmark/benchmark.h>
#include <deque>

#include <meta/api/object.h>
#include <meta/base/namespace.h>

#include "property_utils.h"

META_BEGIN_NAMESPACE()
namespace benchmarks {

void ConstructObject(benchmark::State& state)
{
    auto& objr = GetObjectRegistry();
    for (auto _ : state) {
        auto obj = objr.Create(ClassId::BenchmarkType);
        state.PauseTiming();
        benchmark::DoNotOptimize(obj->GetName());
        state.ResumeTiming();
    }
}

void ConstructProperty(benchmark::State& state)
{
    for (auto _ : state) {
        auto property = META_NS::ConstructProperty<int>("Property");
        state.PauseTiming();
        benchmark::DoNotOptimize(property->GetValue());
        state.ResumeTiming();
    }
}

void ManyObjects(benchmark::State& state)
{
    constexpr size_t objectCount = 50000;
    auto& objr = GetObjectRegistry();
    objr.Purge(); // make sure there is nothing from other tests
    for (auto _ : state) {
        std::deque<IObject::Ptr> objects;
        for (size_t i = 0; i != objectCount; ++i) {
            objects.push_back(objr.Create(ClassId::BenchmarkType));
        }
    }
}

void ManyObjectsWithAddAndRemove(benchmark::State& state)
{
    constexpr size_t objectCount = 50000;
    auto& objr = GetObjectRegistry();
    objr.Purge();
    std::deque<IObject::Ptr> objects;
    for (size_t i = 0; i != objectCount; ++i) {
        // reverse, so when we remove from the back, it is the last one created
        objects.push_front(objr.Create(ClassId::BenchmarkType));
    }
    for (auto _ : state) {
        for (size_t i = 0; i != objectCount; ++i) {
            objects.pop_back();
            objects.push_back(objr.Create(ClassId::BenchmarkType));
        }
    }
}

BENCHMARK(ConstructObject);
BENCHMARK(ConstructProperty);
BENCHMARK(ManyObjects);
BENCHMARK(ManyObjectsWithAddAndRemove);

} // namespace benchmarks
META_END_NAMESPACE()
