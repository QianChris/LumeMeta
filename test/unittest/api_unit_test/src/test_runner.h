/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef META_TEST_RUNNER
#define META_TEST_RUNNER

#include <base/util/uid.h>
#include <core/intf_engine.h>
#include <core/io/intf_filesystem_api.h>
#include <core/os/intf_platform.h>
#include <core/os/platform_create_info.h>
#include <core/plugin/intf_plugin.h>

#include <meta/base/namespace.h>
#include <meta/base/plugin.h>

#include "helpers/testing_objects.h"
#include "test_framework.h"

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
#include <core/plugin/intf_plugin_register.h>

#include "load_lib.h"
#endif

#if defined(__ANDROID__)
#include <test_runner_android.h>
#elif defined(__OHOS__)
#include <test_runner_ohos.h>
#elif defined(_WIN32)
#include <test_runner_windows.h>
#undef OPTIONAL
#undef CreateDirectory
#undef CreateFile
#undef GetClassName
#elif defined(__linux__)
#include <test_runner_linux.h>
#endif

// Just making sure that RTTI is disabled. Remove this if we decide to enable RTTI at some point.
#if defined(_CPPRTTI) || defined(__GXX_RTTI)
#error RTTI not disabled
#endif

META_BEGIN_NAMESPACE()
namespace UTest {

struct TestEnvironment {
    CORE_NS::PlatformCreateInfo platformCreateInfo;
    BASE_NS::string appAssetPath;
    CORE_NS::IEngine::Ptr engine;
};

TestEnvironment* GetTestEnv();
void RegisterPaths(CORE_NS::IFileManager& fileManager, CORE_NS::IEngine& engine);
void RegisterTestFileProtocol(CORE_NS::IFileManager& fileManager);
CORE_NS::IEngine::Ptr CreateEngine();

namespace {

std::unique_ptr<TestEnvironment> g_testEnv{};

class TestRunnerEnv : public ::testing::Environment {
public:
    TestRunnerEnv(int argc, char** argv)
    {}

    void SetUp() override
    {
#if defined(__ANDROID__)
        const CORE_NS::PlatformCreateInfo info{::Test::g_androidApp->GetEnv(), *(::Test::g_androidApp->GetContext())};
        const BASE_NS::string appAssetPath = "";
#elif defined(__OHOS__)
        const CORE_NS::PlatformCreateInfo info{::Test::g_ohosApp->GetEnv(),
            ::Test::g_ohosApp->GetContext(),
            nullptr,
            ::Test::g_ohosApp->GetRsManager(),
            ::Test::g_ohosApp->GetNativeLibDir()};
        const BASE_NS::string appAssetPath = ::Test::g_ohosApp->GetAssetDir();
#elif defined(_WIN32)
        const CORE_NS::PlatformCreateInfo info{::Test::g_windowsApp->GetEngineDir(),
            ::Test::g_windowsApp->GetAppDir(),
            ::Test::g_windowsApp->GetPluginsDir()};

        const BASE_NS::string appAssetPath = ::Test::g_windowsApp->GetAssetDir();
#elif defined(__linux__)
        const CORE_NS::PlatformCreateInfo info{
            ::Test::g_linuxApp->GetEngineDir(), ::Test::g_linuxApp->GetAppDir(), ::Test::g_linuxApp->GetPluginsDir()};

        const BASE_NS::string appAssetPath = ::Test::g_linuxApp->GetAssetDir();
#endif

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
        // Load engine lib
#if defined(_WIN32)
        m_engineLib.Load("AGPEngineDLL");
#elif defined(__ANDROID__) || defined(__OHOS__)
        m_engineLib.Load("libAGPEngineDLL");
#else  // Linux
        m_engineLib.Load("./libAGPEngineDLL");
#endif

        ASSERT_TRUE(m_engineLib.IsLoaded());
        // Load functions
#if defined(_WIN32)
        ASSERT_TRUE(m_engineLib.LoadFunction<decltype(CORE_NS::CreatePluginRegistry)>(
            CORE_NS::CreatePluginRegistry, "?CreatePluginRegistry@Core@@YAXAEBUPlatformCreateInfo@1@@Z"));
        ASSERT_TRUE(m_engineLib.LoadFunction<decltype(CORE_NS::GetPluginRegister)>(
            CORE_NS::GetPluginRegister, "?GetPluginRegister@Core@@YAAEAVIPluginRegister@1@XZ"));
#else
        m_engineLib.LoadFunction<decltype(CORE_NS::CreatePluginRegistry)>(
            CORE_NS::CreatePluginRegistry, "_ZN4Core20CreatePluginRegistryERKNS_18PlatformCreateInfoE");
        m_engineLib.LoadFunction<decltype(CORE_NS::GetPluginRegister)>(
            CORE_NS::GetPluginRegister, "_ZN4Core17GetPluginRegisterEv");
#endif
#endif  // defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)

        CORE_NS::CreatePluginRegistry(info);

        constexpr BASE_NS::Uid uids[]{META_NS::META_OBJECT_PLUGIN_UID};
        ASSERT_TRUE(CORE_NS::GetPluginRegister().LoadPlugins(uids));

        g_testEnv = std::make_unique<TestEnvironment>();
        g_testEnv->platformCreateInfo = info;
        g_testEnv->appAssetPath = appAssetPath;
        g_testEnv->engine = CreateEngine();

        RegisterTestTypes();
    }

    void TearDown() override
    {
        UnregisterTestTypes();
        g_testEnv.reset();
    }

private:
#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
    Test::DynamicLibrary m_engineLib;
#endif
};
}  // namespace

}  // namespace UTest
META_END_NAMESPACE()

#endif  // META_TEST_RUNNER
