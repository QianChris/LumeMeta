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

#include "test_runner_linux.h"

#include <algorithm>  // std::replace
#include <cstring>    // memcpy
#include <filesystem>
#include <iostream>  // cout

#include <base/containers/unique_ptr.h>

namespace Test {

LinuxApp::LinuxApp(int argc, char** argv)
{
    ParseArguments(argc, argv);

    // Save args for GTest
    for (int i = 0; i < argc; ++i) {
        m_testData.gtestArgs.emplace_back(argv[i]);
    }
}

LinuxApp::~LinuxApp()
{
    Terminate();
}

void LinuxApp::Init()
{}

void LinuxApp::Run()
{
    // Run tests
    m_testData.result = RunGoogleTest(m_testData.gtestArgs.size(), m_testData.gtestArgs.data());
}

void LinuxApp::Terminate()
{
    if (m_xcbConn != nullptr) {
        DestroyNativeWindow();
    }
}

BASE_NS::string LinuxApp::GetCPUVendor() const
{
    int regs[4] = {0};
    char vendor[13];
    __asm__ __volatile__("cpuid" : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3]) : "a"(0));
    std::memcpy(vendor, &regs[1], 4);      // EBX
    std::memcpy(vendor + 4, &regs[3], 4);  // EDX
    std::memcpy(vendor + 8, &regs[2], 4);  // ECX
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

void LinuxApp::PrintPaths()
{
    PrintPaths("Current working dir: ", std::filesystem::current_path().generic_string().c_str());
    PrintPaths("Engine dir: ", m_lumeEngineRootPath);
    PrintPaths("App dir: ", m_lumeAppPath);
    PrintPaths("Plugin dir: ", m_lumePluginPath);
    PrintPaths("App assets dir: ", m_lumeAppAssetPath);
}

void LinuxApp::PrintPaths(const BASE_NS::string& msg, const BASE_NS::string& path)
{
    BASE_NS::string str = "\x1b[1m" + msg + "\x1b[0m";
    if (std::filesystem::path(path.c_str()).is_absolute()) {
        std::cout << str.c_str() << std::filesystem::path(path.c_str()).lexically_normal() << std::endl;
    } else {
        std::cout << str.c_str() << (std::filesystem::current_path() / path.c_str()).lexically_normal() << std::endl;
    }
}

void LinuxApp::CreateNativeWindow(uint32_t width, uint32_t height, const BASE_NS::string& title)
{
    // Destroy current window if it exists
    if (m_xcbConn != nullptr) {
        DestroyNativeWindow();
    }

    // Open the connection to the X server
    int screen_id;
    m_xcbConn = xcb_connect(nullptr, &screen_id);
    if (xcb_connection_has_error(m_xcbConn)) {
        CORE_LOG_F("Could not connect to X server.");
        return;
    }
    m_xcbWin = xcb_generate_id(m_xcbConn);

    // Get screen data
    xcb_screen_t* screen;
    auto iter = xcb_setup_roots_iterator(xcb_get_setup(m_xcbConn));
    for (; iter.rem; --screen_id, xcb_screen_next(&iter)) {
        if (screen_id == 0) {
            screen = iter.data;
            break;
        }
    }

    // Create window
    xcb_create_window(m_xcbConn,  // Connection
        XCB_COPY_FROM_PARENT,     // Depth
        m_xcbWin,                 // Window id
        screen->root,             // Parent window
        0,
        0,  // x, y position
        width,
        height,                         // width, height
        10,                             // border_width
        XCB_WINDOW_CLASS_INPUT_OUTPUT,  // Class
        screen->root_visual,            // Visual
        0,
        nullptr);  // masks

    // Set title
    xcb_change_property(
        m_xcbConn, XCB_PROP_MODE_REPLACE, m_xcbWin, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, title.size(), title.data());

    // Map the window on the screen (make it visible)
    // If we don't map the window it's invisible but drawing to it is still possible.
    // xcb_map_window(m_xcbConn, m_xcbWin);

    // Flush the connection to ensure the changes are applied
    xcb_flush(m_xcbConn);
}

void LinuxApp::DestroyNativeWindow()
{
    xcb_destroy_window(m_xcbConn, m_xcbWin);
    xcb_disconnect(m_xcbConn);
    m_xcbConn = nullptr;
}

xcb_connection_t* LinuxApp::GetWindowConnection()
{
    return m_xcbConn;
}

xcb_window_t LinuxApp::GetWindowID() const
{
    return m_xcbWin;
}

int LinuxApp::GetGTestResult() const
{
    return m_testData.result;
}

BASE_NS::string LinuxApp::GetEngineDir() const
{
    return m_lumeEngineRootPath;
}

BASE_NS::string LinuxApp::GetAppDir() const
{
    return m_lumeAppPath;
}

BASE_NS::string LinuxApp::GetPluginsDir() const
{
    return m_lumePluginPath;
}

BASE_NS::string LinuxApp::GetAssetDir() const
{
    return m_lumeAppAssetPath;
}

void LinuxApp::ParseArguments(int argc, char** argv)
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

}  // namespace Test

int main(int argc, char** argv)
{
    BASE_NS::unique_ptr<Test::TestApp> app = BASE_NS::make_unique<Test::LinuxApp>(argc, argv);
    // WARN: No dynamic_cast since it requires RTTI
    Test::g_linuxApp = static_cast<Test::LinuxApp*>(app.get());

    Test::g_linuxApp->Init();
    Test::g_linuxApp->Run();
    int result = Test::g_linuxApp->GetGTestResult();
    Test::g_linuxApp->Terminate();

    return result;
}