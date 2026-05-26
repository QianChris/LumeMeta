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

#ifndef TEST_RUNNER_WINDOWS
#define TEST_RUNNER_WINDOWS

#include <base/containers/string.h>
#include <base/containers/vector.h>

#include "test_runner_common.h"

namespace Test {

class WindowsApp : public TestApp {
public:
    WindowsApp(int argc, char** argv);
    ~WindowsApp();

    void Init() override;

    void Run() override;

    void Terminate() override;

    BASE_NS::string GetCPUVendor() const;

    void PrintPaths();

    void CreateNativeWindow(uint32_t width, uint32_t height, const BASE_NS::string& title = "TestWindow");

    void DestroyNativeWindow();

    void* GetWindowHandle() const;

    int GetGTestResult() const;

    BASE_NS::string GetEngineDir() const;

    BASE_NS::string GetAppDir() const;

    BASE_NS::string GetPluginsDir() const;

    BASE_NS::string GetAssetDir() const;

private:
    void ParseArguments(int argc, char** argv);

    void PrintPaths(const BASE_NS::string& msg, const BASE_NS::string& path);

    struct TestData {
        BASE_NS::vector<char*> gtestArgs;  // Input data for Gtest
        BASE_NS::string reportTestFile;    // Report log file
        BASE_NS::string gtestTestFile;     // Gtest results file
        int result = -1;                   // Gtest results exit code
        bool hasRunTests = false;
    };

    BASE_NS::string m_lumeEngineRootPath{"./"};
    BASE_NS::string m_lumeAppPath{"./"};
    BASE_NS::string m_lumePluginPath{"./plugins/"};
    BASE_NS::string m_lumeAppAssetPath{"./../../assets/test_data/"};

    // Window
    void* m_hWnd{nullptr};
    void* m_hDC{nullptr};

    // GTest
    TestData m_testData;
};

inline WindowsApp* g_windowsApp;

}  // namespace Test
#endif  // TEST_RUNNER_WINDOWS
