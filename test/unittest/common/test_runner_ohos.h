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

#ifndef TEST_RUNNER_OHOS
#define TEST_RUNNER_OHOS

// Test runner for HarmonyOS / OpenHarmony.
//
// The unit tests are built as plain executables, pushed to a device and run
// directly from a shell. There is no HAP, no XComponent and no JS environment:
// the napi handles below are null (the ohos_ndk platform backend only stores
// them, it never dereferences them).
//
// Nothing about the layout is compiled in, and the defaults are the same ones the
// desktop runners use: the build already arranges the engine, the plugins and the
// test data around each test executable, so running a suite is just a matter of
// changing into its directory and starting it, exactly as on Linux.
//
//   plugins    LUME_PLUGIN_DIR,    default <exe dir>/plugins
//   test data  LUME_TEST_DATA_DIR, default <exe dir>/../../assets/test_data
//   files,     LUME_TEST_WORK_DIR, default <exe dir>
//   cache
//
// The engine itself (libAGPEngineDLL) is dlopen()ed by bare name and so comes
// from LD_LIBRARY_PATH / RUNPATH; it is deliberately not looked up here. Note
// that it is *not* the libAGPDLL.z.so that ships in the device image, so there
// is no way to silently fall back to the system engine: if the deployed engine
// is missing the load fails loudly instead.

#include <napi/native_api.h>
#include <rawfile/raw_file_manager.h>
#include <unistd.h>

#include <cstdlib>
#include <string>

#include <base/containers/string.h>

#include "test_runner_common.h"

namespace Test {

// Directory holding the running executable. Used only as a default, so that a
// self-contained "everything next to the binary" layout works with no
// configuration at all.
inline BASE_NS::string ExecutableDir()
{
    char buf[4096];
    const ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len <= 0) {
        return ".";
    }
    buf[len] = '\0';
    std::string path(buf);
    const auto slash = path.find_last_of('/');
    return BASE_NS::string((slash == std::string::npos) ? "." : path.substr(0, slash).c_str());
}

inline BASE_NS::string EnvOr(const char* name, const BASE_NS::string& fallback)
{
    const char* value = std::getenv(name);
    return (value && *value) ? BASE_NS::string(value) : fallback;
}

class OHOSApp : public TestApp {
public:
    OHOSApp() = default;
    ~OHOSApp() override = default;

    void Init() override
    {}
    void Run() override
    {}
    void Terminate() override
    {}

    // No JS environment in a standalone process.
    const napi_env& GetEnv() const
    {
        return m_env;
    }
    const napi_value& GetContext() const
    {
        return m_context;
    }
    // No HAP, so no resource manager -- assets are reached over file:// instead.
    NativeResourceManager* GetRsManager()
    {
        return nullptr;
    }

    // Becomes PlatformCreateInfo::appPluginPath, which the platform registers as
    // the plugins:// search path.
    const BASE_NS::string& GetNativeLibDir() const
    {
        return m_pluginDir;
    }
    const BASE_NS::string& GetAssetDir() const
    {
        return m_assetDir;
    }
    const BASE_NS::string& GetCacheDir() const
    {
        return m_cacheDir;
    }
    const BASE_NS::string& GetFilesDir() const
    {
        return m_filesDir;
    }

private:
    napi_env m_env{nullptr};
    napi_value m_context{nullptr};

    // Absolute: the platform turns these into file:/// URIs, which a relative
    // path would corrupt. Derived from the executable so that the layout the
    // build already produces works with no configuration.
    BASE_NS::string m_pluginDir{EnvOr("LUME_PLUGIN_DIR", ExecutableDir() + "/plugins") + "/"};
    BASE_NS::string m_assetDir{EnvOr("LUME_TEST_DATA_DIR", ExecutableDir() + "/../../assets/test_data") + "/"};
    BASE_NS::string m_workDir{EnvOr("LUME_TEST_WORK_DIR", ExecutableDir())};
    // As on the desktop runners: the application directory is where the test
    // runs, and the cache sits under it (the engine creates it on demand).
    BASE_NS::string m_filesDir{m_workDir};
    BASE_NS::string m_cacheDir{m_workDir + "/cache"};
};

inline OHOSApp ohosApp;
inline OHOSApp* g_ohosApp = &ohosApp;

}  // namespace Test

#endif  // TEST_RUNNER_OHOS
