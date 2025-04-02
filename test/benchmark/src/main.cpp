#include <benchmark/benchmark.h>
#include <meta/base/plugin.h>

#include <core/engine_info.h>
#include <core/os/intf_platform.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/plugin/intf_class_register.h>
#include <core/plugin/intf_plugin_register.h>

#include "utils.h"

namespace benchmarks {

class BenchmarkEnvironment {
public:
    BenchmarkEnvironment()
    {
        const CORE_NS::PlatformCreateInfo info { "./", "./", "./plugins" }; 
        CORE_NS::CreatePluginRegistry(info);
        constexpr BASE_NS::Uid uids[] { META_NS::META_OBJECT_PLUGIN_UID };
        CORE_NS::GetPluginRegister().LoadPlugins(uids);

        CORE_NS::VersionInfo versInfoEngine {
            "MetaObject_Benchmark_Runner",
            0,
            1,
            0,
        };
        const CORE_NS::EngineCreateInfo engineCreateInfo { { "./", "./", "" }, versInfoEngine, {} };

        auto factory = CORE_NS::GetInstance<CORE_NS::IEngineFactory>(CORE_NS::UID_ENGINE_FACTORY);
        engine_ = factory->Create(engineCreateInfo);
        engine_->Init();
    }

    ~BenchmarkEnvironment()
    {
        engine_.reset();
    }

private:
    CORE_NS::IEngine::Ptr engine_;
};

} // namespace benchmarks

int main(int argc, char** argv)
{
    const auto environment = benchmarks::BenchmarkEnvironment();

    META_NS::benchmarks::RegisterBenchmarkTypes();

    benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();

    META_NS::benchmarks::UnregisterBenchmarkTypes();

    return 0;
}
