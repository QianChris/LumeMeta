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

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <memory>
#include <mutex>
#include <napi/native_api.h>
#include <rawfile/raw_file_manager.h>

#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>

#include "test_runner_common.h"

namespace Test {

// xComponent callbacks
class IXCCallbacks {
public:
    virtual ~IXCCallbacks();
    static IXCCallbacks* GetInstance(OH_NativeXComponent* c);
    static void Hook(OH_NativeXComponent* c, IXCCallbacks* handler);
    static void Unhook(IXCCallbacks* handler);
    static void UnhookAll();

protected:
    static BASE_NS::string GetXCN(OH_NativeXComponent* c);
    static void OnSurfaceCreatedCB(OH_NativeXComponent* component, void* window);
    static void OnSurfaceChangedCB(OH_NativeXComponent* component, void* window);
    static void OnSurfaceDestroyedCB(OH_NativeXComponent* component, void* window);
    static void DispatchTouchEventCB(OH_NativeXComponent* component, void* window);

    OH_NativeXComponent* m_component{nullptr};
    void* m_window{nullptr};
    virtual void OnSurfaceChanged() = 0;
    virtual void OnSurfaceDestroyed() = 0;
    virtual void DispatchTouchEvent() = 0;
    virtual void OnSurfaceCreated() = 0;
    static OH_NativeXComponent_Callback m_cb;
    static BASE_NS::unordered_map<BASE_NS::string, BASE_NS::unique_ptr<IXCCallbacks>> m_instances;
};

inline BASE_NS::unordered_map<BASE_NS::string, BASE_NS::unique_ptr<IXCCallbacks>> IXCCallbacks::m_instances;
inline OH_NativeXComponent_Callback IXCCallbacks::m_cb{IXCCallbacks::OnSurfaceCreatedCB,
    IXCCallbacks::OnSurfaceChangedCB,
    IXCCallbacks::OnSurfaceDestroyedCB,
    IXCCallbacks::DispatchTouchEventCB};

// xComponent impl for Gtests
class OHOSApp : public TestApp, public IXCCallbacks {
public:
    OHOSApp(napi_env env, napi_value exports) : m_env(env), m_exports(exports)
    {
        Init();
    }
    ~OHOSApp()
    {}

    void OnSurfaceCreated() override;
    void OnSurfaceChanged() override;
    void OnSurfaceDestroyed() override;
    void DispatchTouchEvent() override;

    void Init() override;
    void Run() override;
    void Terminate() override;

    void* GetWindowHandle();
    int GetGTestResult();
    const napi_env& GetEnv() const;
    const napi_value& GetContext() const;
    NativeResourceManager* GetRsManager();
    const BASE_NS::string& GetCacheDir() const;
    const BASE_NS::string& GetFilesDir() const;
    const BASE_NS::string& GetTmpDir() const;
    const BASE_NS::string& GetNativeLibDir() const;

private:
    static napi_value TerminateCB(napi_env env, napi_callback_info info);

    void SetPaths();
    void SetGtestEnv();
    BASE_NS::string GetNapiValueAsString(const BASE_NS::string& propName) const;
    void CleanGtestEnv();

    struct TestData {
        std::mutex mtx;                    // Mutex
        BASE_NS::vector<char*> gtestArgs;  // Input data for Gtest
        BASE_NS::string reportTestFile;    // Report log file
        BASE_NS::string gtestTestFile;     // Gtest results file
        int result = -1;                   // Gtest results exit code
    };
    napi_env m_env{nullptr};
    napi_value m_exports{nullptr};
    napi_value m_context{nullptr};
    napi_threadsafe_function m_termFun{nullptr};
    NativeResourceManager* m_nativeRsManager{nullptr};
    BASE_NS::string m_cacheDir, m_filesDir, m_tempDir, m_nativeLibDir;
    TestData m_testData;
};

inline OHOSApp* g_ohosApp;

}  // namespace Test

#endif  // TEST_RUNNER_OHOS
