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

#include "test_runner_ohos.h"

#include <thread>

namespace Test {

//
// xComponent callbacks impl
//
IXCCallbacks::~IXCCallbacks()
{
    UnhookAll();
}

IXCCallbacks* IXCCallbacks::GetInstance(OH_NativeXComponent* c)
{
    auto it = m_instances.find(GetXCN(c));
    if (it == m_instances.end()) {
        return nullptr;
    }
    return it->second.get();
}

void IXCCallbacks::Hook(OH_NativeXComponent* c, IXCCallbacks* handler)
{
    auto name = GetXCN(c);
    if (m_instances[name].get() != handler) {
        m_instances[name].reset(handler);
    }

    handler->m_component = c;
    OH_NativeXComponent_RegisterCallback(c, &IXCCallbacks::m_cb);
}

void IXCCallbacks::Unhook(IXCCallbacks* handler)
{
    for (auto it = m_instances.begin(); it != m_instances.end(); ++it) {
        if (it->second.get() == handler) {
            // is this a correct way to unregister the callbacks?
            OH_NativeXComponent_RegisterCallback(it->second->m_component, nullptr);
            m_instances.erase(it);
            return;
        }
    }
}

void IXCCallbacks::UnhookAll()
{
    m_instances.clear();
}

BASE_NS::string IXCCallbacks::GetXCN(OH_NativeXComponent* c)
{
    char idstr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
    uint64_t idlen = OH_XCOMPONENT_ID_LEN_MAX + 1;
    OH_NativeXComponent_GetXComponentId(c, idstr, &idlen);
    return {idstr, idlen};
}

void IXCCallbacks::OnSurfaceCreatedCB(OH_NativeXComponent* component, void* window)
{
    auto cb = GetInstance(component);
    if (cb) {
        cb->m_window = window;
        cb->OnSurfaceCreated();
    }
}

void IXCCallbacks::OnSurfaceChangedCB(OH_NativeXComponent* component, void* window)
{
    auto cb = GetInstance(component);
    if (cb) {
        cb->m_window = window;
        cb->OnSurfaceChanged();
    }
}

void IXCCallbacks::OnSurfaceDestroyedCB(OH_NativeXComponent* component, void* window)
{
    auto cb = GetInstance(component);
    if (cb) {
        cb->m_window = window;
        cb->OnSurfaceDestroyed();
    }
}

void IXCCallbacks::DispatchTouchEventCB(OH_NativeXComponent* component, void* window)
{
    auto cb = GetInstance(component);
    if (cb) {
        cb->m_window = window;
        cb->DispatchTouchEvent();
    }
}

//
// xComponent impl for gtests impl
//
void OHOSApp::OnSurfaceCreated()
{
    // Set gtest environment
    SetGtestEnv();

    // xComponent prop.
    {
        auto name = GetXCN(m_component).c_str();
        uint64_t width;
        uint64_t height;
        int32_t xSize = OH_NativeXComponent_GetXComponentSize(m_component, m_window, &width, &height);
    }

    // Spawn a background thread to run tests
    std::thread bgThread(&OHOSApp::Run, this);

    // Detach the background thread
    bgThread.detach();
}

napi_value OHOSApp::TerminateCB(napi_env env, napi_callback_info info)
{
    LOGI("TerminateCB...");
    /*if (auto *ris = RuntimeInstanceState::Get(env)) {
        napi_set_instance_data(env, nullptr, nullptr, nullptr); // THIS triggers the finalize...
    }*/
    return nullptr;
}

void OHOSApp::OnSurfaceChanged()
{}
void OHOSApp::OnSurfaceDestroyed()
{
    LOGI("OnSurfaceDestroyed...");
}
void OHOSApp::DispatchTouchEvent()
{}

void* OHOSApp::GetWindowHandle()
{
    return m_window;
}

const napi_env& OHOSApp::GetEnv() const
{
    return m_env;
}

const napi_value& OHOSApp::GetContext() const
{
    return m_context;
}

NativeResourceManager* OHOSApp::GetRsManager()
{
    return m_nativeRsManager;
}

const BASE_NS::string& OHOSApp::GetCacheDir() const
{
    return m_cacheDir;
}

const BASE_NS::string& OHOSApp::GetFilesDir() const
{
    return m_filesDir;
}

const BASE_NS::string& OHOSApp::GetTmpDir() const
{
    return m_tempDir;
}

const BASE_NS::string& OHOSApp::GetNativeLibDir() const
{
    return m_nativeLibDir;
}

// Thread-shared data
int OHOSApp::GetGTestResult()
{
    std::lock_guard<std::mutex> lock(m_testData.mtx);
    return m_testData.result;
}

void OHOSApp::Init()
{
    // Get context
    napi_value nGlobal;
    napi_get_global(m_env, &nGlobal);

    napi_value nFunc;
    napi_get_named_property(m_env, nGlobal, "getContext", &nFunc);

    napi_value nArgv[] = {m_exports};
    napi_call_function(m_env, nGlobal, nFunc, 1, nArgv, &m_context);

    // Create function to terminate application from c++ side
    {
        napi_status status;
        napi_value nameVal;
        napi_create_string_latin1(m_env, "Terminate_Runtime", NAPI_AUTO_LENGTH, &nameVal);
        status = napi_create_threadsafe_function(
            m_env,
            nullptr,
            nullptr,
            nameVal,
            0 /*unlimited queue*/,
            1,
            nullptr,
            [](napi_env env, void* finalize_data, void* finalize_hint) {},
            nullptr,
            [](napi_env env, napi_value js_callback, void* context, void* data) {
                napi_status status;
                napi_value nGlobal = nullptr;
                status = napi_get_global(env, &nGlobal);

                napi_value getCtx = nullptr;
                status = napi_get_named_property(env, nGlobal, "getContext", &getCtx);

                napi_value exp = nullptr;
                napi_value nArgv[] = {exp};
                napi_value nObj = nullptr;
                status = napi_call_function(env, nGlobal, getCtx, 1, nArgv, &nObj);

                napi_value term = nullptr;
                status = napi_get_named_property(env, nObj, "terminateSelf", &term);

                napi_value fn = nullptr;
                status = napi_create_function(env, nullptr, 0, TerminateCB, nullptr, &fn);

                napi_value nArg[] = {fn};
                napi_value nObj2 = nullptr;
                status = napi_call_function(env, nObj, term, 1, nArg, &nObj2);
            },
            &m_termFun);
    }

    // Set dirs (cache, files, tmp, ...)
    SetPaths();
}

void OHOSApp::Terminate()
{
    if (m_termFun) {
        napi_call_threadsafe_function(m_termFun, nullptr, napi_threadsafe_function_call_mode::napi_tsfn_nonblocking);
        napi_release_threadsafe_function(m_termFun, napi_tsfn_release);
    }
    m_termFun = nullptr;
}

void OHOSApp::Run()
{
    std::unique_lock<std::mutex> lock(m_testData.mtx);

    // Start redirecting report to a file
    Test::StartOutputFileRedirect(m_testData.reportTestFile.c_str(), false);

    // Run tests
    m_testData.result = RunGoogleTest(m_testData.gtestArgs.size(), m_testData.gtestArgs.data());

    // Stop redirecting report to a file
    Test::StopOutputFileRedirect();

    // Dump report to stdout
    Test::PrintLogFromFile(m_testData.reportTestFile.c_str());

    // Clean GtestEnv
    CleanGtestEnv();

    // Terminate App
    Terminate();
}

void OHOSApp::SetPaths()
{
    // Get resource manager
    napi_value nRsManager;
    napi_get_named_property(m_env, m_context, "resourceManager", &nRsManager);
    m_nativeRsManager = OH_ResourceManager_InitNativeResourceManager(m_env, nRsManager);

    // Get cache dir
    m_cacheDir = GetNapiValueAsString("cacheDir");

    // Get files dir
    m_filesDir = GetNapiValueAsString("filesDir");

    // Get tmp dir
    m_tempDir = GetNapiValueAsString("tempDir");

    // Get bundle lib dir
    m_nativeLibDir = GetNapiValueAsString("bundleCodeDir") + "/libs/arm64";
}

void OHOSApp::SetGtestEnv()
{
    m_testData.reportTestFile = m_filesDir + "/report-All.txt";
    m_testData.gtestTestFile = m_filesDir + "/gtestReport_OHOS.xml";
    m_testData.gtestArgs.emplace_back(new char[20]{"CoreTestRunner"});
    // Mutable copy char*:
    BASE_NS::string tmpStr = "--gtest_output=xml:" + m_testData.gtestTestFile;
    {
        char* temp = new char[tmpStr.size() + 1];
        std::strcpy(temp, tmpStr.data());
        m_testData.gtestArgs.emplace_back(temp);
    }
}

void OHOSApp::CleanGtestEnv()
{
    // Cleanup
    for (auto& arg : m_testData.gtestArgs) {
        delete[] arg;
    }
}

BASE_NS::string OHOSApp::GetNapiValueAsString(const BASE_NS::string& propName) const
{
    size_t size;
    char* buf = nullptr;
    napi_value nVal;
    BASE_NS::string outVal;
    napi_get_named_property(m_env, m_context, propName.c_str(), &nVal);
    napi_get_value_string_utf8(m_env, nVal, nullptr, 0, &size);
    buf = new char[size + 1];
    napi_get_value_string_utf8(m_env, nVal, buf, size + 1, &size);
    outVal = BASE_NS::string(buf);
    delete[] buf;
    buf = nullptr;
    return outVal;
}

}  // namespace Test

EXTERN_C_START
static napi_value Init(napi_env nEnv, napi_value nExports)
{
    // xComponent
    napi_value nVal;
    napi_valuetype nType;
    napi_get_named_property(nEnv, nExports, OH_NATIVE_XCOMPONENT_OBJ, &nVal);
    napi_typeof(nEnv, nVal, &nType);
    if (nType != napi_undefined) {
        OH_NativeXComponent* c;
        napi_unwrap(nEnv, nVal, reinterpret_cast<void**>(&c));

        Test::g_ohosApp = new Test::OHOSApp(nEnv, nExports);
        Test::IXCCallbacks::Hook(c, Test::g_ohosApp);
        Test::g_ohosApp->Init();
    }

    return nExports;
}
EXTERN_C_END

static napi_module nativeModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void*)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    napi_module_register(&nativeModule);
}
