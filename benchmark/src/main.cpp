#include <benchmark/benchmark.h>
#include <meta_object_initializer.h>

#include <core/engine_info.h>
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
        environmentSetup::InitializeMetaObject(".\\AGPEngineDLL.dll", CORE_NS::ILogger::LogLevel::LOG_ERROR);

        CORE_NS::GetPluginRegister().LoadPlugins({}); // load all plugins, do we need this?

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
        environmentSetup::ShutdownMetaObject();
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
