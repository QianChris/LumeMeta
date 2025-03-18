/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#include "tester.h"

#include <meta_object_initializer.h>

#include <gtest/gtest.h>

#include <core/engine_info.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/plugin/intf_class_register.h>
#include <core/plugin/intf_plugin_register.h>

#include <meta/base/namespace.h>

#include "testing_objects.h"

META_BEGIN_NAMESPACE()
namespace Test {

using namespace BASE_NS;

static unique_ptr<TestEnvironment> g_testEnv;
static CORE_NS::IEngine::Ptr engine_ { nullptr };

class TestRunnerEnv : public ::testing::Environment {
public:
    unique_ptr<TestEnvironment>* testEnv;

    void SetUp() override
    {
        g_testEnv = make_unique<TestEnvironment>();
        testEnv = &g_testEnv;

        environmentSetup::InitializeMetaObject(".\\AGPEngineDLL.dll");

        CORE_NS::GetPluginRegister().LoadPlugins({}); // load all plugins

        // For now we MUST create an engine instance..
        CORE_NS::VersionInfo versInfoEngine {
            "MetaObject_TestRunner", // name
            0,                       // versionMajor
            1,                       // versionMinor
            0,                       // versionPatch
        };
        const CORE_NS::EngineCreateInfo engineCreateInfo { { "./", "./", "" },
            // applicationVersion
            versInfoEngine,
            // applicationContext
            {} };

        auto& fm = CORE_NS::GetPluginRegister().GetFileManager();
        fm.RegisterPath("assets", "file://./assets/", false); // so we can use "assets://"
        fm.RegisterPath("project", "file://./", false);       // so we can use "project://"

        auto factory = CORE_NS::GetInstance<CORE_NS::IEngineFactory>(CORE_NS::UID_ENGINE_FACTORY);
        engine_ = factory->Create(engineCreateInfo);
        engine_->Init();

        g_testEnv->engine = engine_.get();

        RegisterTestTypes();
    }

    void TearDown() override
    {
        UnregisterTestTypes();
        g_testEnv.reset();

        engine_ = nullptr;
        environmentSetup::ShutdownMetaObject();
    }
};

LogLevelScope::LogLevelScope(CORE_NS::ILogger* aLogger, CORE_NS::ILogger::LogLevel aLogLevel) : mLogger(*aLogger)
{
    mOldLevel = mLogger.GetLogLevel();
    mLogger.SetLogLevel(aLogLevel);
}

LogLevelScope::~LogLevelScope()
{
    mLogger.SetLogLevel(mOldLevel);
}

TestEnvironment* getTestEnv()
{
    CORE_ASSERT(g_testEnv.get());
    return g_testEnv.get();
}

} // namespace Test
META_END_NAMESPACE()

int main(int argc, char** argv)
{
    //_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
    printf("Running main() from TestRunner.cpp\n");
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new META_NS::Test::TestRunnerEnv());
    // Uncomment the following for filtering tests at runtime
    //::testing::GTEST_FLAG(filter) = "UnitTest_AnimationTypelessTest.*";
    return RUN_ALL_TESTS();
}
