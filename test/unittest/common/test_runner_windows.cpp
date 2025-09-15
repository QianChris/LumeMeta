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

#include "test_runner_windows.h"

#include <algorithm> // std::replace
#include <cstring>   // memcpy
#include <filesystem>
#include <intrin.h> // __cpuid
#include <iostream>
#include <memory>
#include <Windows.h> // window

#include <base/containers/unique_ptr.h>

namespace Test {

namespace {
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    switch (msg) {
        case WM_CREATE:
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT:
            ValidateRect(hwnd, nullptr);
            return 0;
    }
    return DefWindowProc(hwnd, msg, w, l);
}
} // namespace

WindowsApp::WindowsApp(int argc, char** argv)
{
    ParseArguments(argc, argv);

    // Save args for GTest
    for (int i = 0; i < argc; ++i) {
        m_testData.gtestArgs.emplace_back(argv[i]);
    }
}

WindowsApp::~WindowsApp()
{
    Terminate();
}

void WindowsApp::Init() {}

void WindowsApp::Run()
{
    // Run tests
    m_testData.result = RunGoogleTest(m_testData.gtestArgs.size(), m_testData.gtestArgs.data());
    m_testData.hasRunTests = true;
}

void WindowsApp::Terminate()
{
    if (m_hWnd != nullptr) {
        DestroyNativeWindow();
    }
}

BASE_NS::string WindowsApp::GetCPUVendor() const
{
    int regs[4] = { 0 };
    char vendor[13];
    __cpuid(regs, 0);
    std::memcpy(vendor, &regs[1], 4);     // EBX
    std::memcpy(vendor + 4, &regs[3], 4); // EDX
    std::memcpy(vendor + 8, &regs[2], 4); // ECX
    vendor[12] = '\0';
    BASE_NS::string cpuVendor(vendor);

    if (cpuVendor == "GenuineIntel") {
        return "Intel";
    } else if (cpuVendor == "AuthenticAMD") {
        return "AMD";
    } else {
        return "Unknown";
    }
}

void WindowsApp::PrintPaths()
{
    PrintPaths("Current working dir: ", std::filesystem::current_path().generic_string().c_str());
    PrintPaths("Engine dir: ", m_lumeEngineRootPath);
    PrintPaths("App dir: ", m_lumeAppPath);
    PrintPaths("Plugin dir: ", m_lumePluginPath);
    PrintPaths("App assets dir: ", m_lumeAppAssetPath);
}

void WindowsApp::PrintPaths(const BASE_NS::string& msg, const BASE_NS::string& path)
{
    BASE_NS::string str = "\x1b[1m" + msg + "\x1b[0m";
    if (std::filesystem::path(path.c_str()).is_absolute()) {
        std::cout << str.c_str() << std::filesystem::path(path.c_str()).lexically_normal() << std::endl;
    } else {
        std::cout << str.c_str() << (std::filesystem::current_path() / path.c_str()).lexically_normal() << std::endl;
    }
}

void WindowsApp::CreateNativeWindow(uint32_t width, uint32_t height, const BASE_NS::string& title)
{
    // Destroy current window if it exists
    if (m_hWnd != nullptr) {
        DestroyNativeWindow();
    }

    HINSTANCE hInstance = GetModuleHandle(nullptr);
    WNDCLASS wc = { 0 };
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("TestClass");
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
    RECT rect = { 0, 0, static_cast<int>(width), static_cast<int>(height) };
    AdjustWindowRect(&rect, style, false);
    m_hWnd = CreateWindowEx(0, wc.lpszClassName, TEXT(title.c_str()), style, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, hInstance, nullptr);
    m_hDC = GetDC((HWND)m_hWnd);
}

void WindowsApp::DestroyNativeWindow()
{
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    ReleaseDC((HWND)m_hWnd, (HDC)m_hDC);
    DestroyWindow((HWND)m_hWnd);
    UnregisterClass(TEXT("TestClass"), hInstance);
    m_hDC = nullptr;
    m_hWnd = nullptr;
}

void* WindowsApp::GetWindowHandle() const
{
    return m_hWnd;
}

int WindowsApp::GetGTestResult() const
{
    return m_testData.result;
}

BASE_NS::string WindowsApp::GetEngineDir() const
{
    return m_lumeEngineRootPath;
}

BASE_NS::string WindowsApp::GetAppDir() const
{
    return m_lumeAppPath;
}

BASE_NS::string WindowsApp::GetPluginsDir() const
{
    return m_lumePluginPath;
}

BASE_NS::string WindowsApp::GetAssetDir() const
{
    return m_lumeAppAssetPath;
}

void WindowsApp::ParseArguments(int argc, char** argv)
{
    // Parse cmd arguments to overwrite PlatformCreateInfo if necessary
    for (int i = 0; i < argc; ++i) {
        BASE_NS::string arg = argv[i];

        if (arg == "-cp" || arg == "core-path") {
            if (i + 1 < argc) {
                m_lumeEngineRootPath = argv[++i];
                std::replace(m_lumeEngineRootPath.begin(), m_lumeEngineRootPath.end(), '\\', '/');
            }
        } else if (arg == "-ap" || arg == "app-path") {
            if (i + 1 < argc) {
                m_lumeAppPath = argv[++i];
                std::replace(m_lumeAppPath.begin(), m_lumeAppPath.end(), '\\', '/');
            }
        } else if (arg == "-pp" || arg == "plugin-path") {
            if (i + 1 < argc) {
                m_lumePluginPath = argv[++i];
                std::replace(m_lumePluginPath.begin(), m_lumePluginPath.end(), '\\', '/');
            }
        } else if (arg == "-aap" || arg == "app-asset-path") {
            if (i + 1 < argc) {
                m_lumeAppAssetPath = argv[++i];
                std::replace(m_lumeAppAssetPath.begin(), m_lumeAppAssetPath.end(), '\\', '/');
            }
        }
    }
}

} // namespace Test

int main(int argc, char** argv)
{
    BASE_NS::unique_ptr<Test::TestApp> app = BASE_NS::make_unique<Test::WindowsApp>(argc, argv);
    // WARN: No dynamic_cast since it requires RTTI
    Test::g_windowsApp = static_cast<Test::WindowsApp*>(app.get());

    Test::g_windowsApp->Init();
    Test::g_windowsApp->Run();
    int result = Test::g_windowsApp->GetGTestResult();
    Test::g_windowsApp->Terminate();

    return result;
}