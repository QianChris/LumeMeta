#include <benchmark/benchmark.h>
#include <deque>

#include <meta/base/namespace.h>

#include "utils.h"

META_BEGIN_NAMESPACE()
namespace benchmarks {

void SharedPtrConstruct(benchmark::State& state)
{
    auto object = CreateBenchmarkType();
    for (auto _ : state) {
        BASE_NS::shared_ptr<IBenchmarkType> p(object.get());
        benchmark::DoNotOptimize(p);
    }
}

void SharedPtrCopy(benchmark::State& state)
{
    auto object = CreateBenchmarkType();
    for (auto _ : state) {
        BASE_NS::shared_ptr<IBenchmarkType> p(object);
        benchmark::DoNotOptimize(p);
    }
}

void SharedPtrMove(benchmark::State& state)
{
    auto object = CreateBenchmarkType();
    for (auto _ : state) {
        BASE_NS::shared_ptr<IBenchmarkType> p(BASE_NS::move(object));
        object = BASE_NS::move(p);
        benchmark::DoNotOptimize(p);
    }
}

void WeakPtrConstruct(benchmark::State& state)
{
    auto object = CreateBenchmarkType();
    for (auto _ : state) {
        BASE_NS::weak_ptr<IBenchmarkType> p(object);
        benchmark::DoNotOptimize(p);
    }
}

void WeakPtrLock(benchmark::State& state)
{
    auto object = CreateBenchmarkType();
    BASE_NS::weak_ptr<IBenchmarkType> weak(object);
    for (auto _ : state) {
        auto p = weak.lock();
        benchmark::DoNotOptimize(p);
    }
}

BENCHMARK(SharedPtrConstruct);
BENCHMARK(SharedPtrCopy);
BENCHMARK(SharedPtrMove);
BENCHMARK(WeakPtrConstruct);
BENCHMARK(WeakPtrLock);

} // namespace benchmarks
META_END_NAMESPACE()
