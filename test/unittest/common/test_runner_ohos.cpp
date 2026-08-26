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

#include <unistd.h>

#include <cstdio>

#include "test_runner_ohos.h"

// Entry point for the OHOS test runner.
//
// Deliberately NOT gtest_main: RunGoogleTest() also registers the TestRunnerEnv
// global environment, and with CORE_DYNAMIC=1 that environment is what dlopen()s
// the engine and resolves CreatePluginRegistry/GetPluginRegister. Skipping it
// leaves those pointers null.
int main(int argc, char** argv)
{
    // Echo the resolved locations. Everything is resolved at run time, so this is
    // the only record of where the run actually read its engine, plugins and
    // assets from -- worth having in a CI log when a result looks wrong.
    printf("[runner] plugins   : %s\n", Test::g_ohosApp->GetNativeLibDir().c_str());
    printf("[runner] test data : %s\n", Test::g_ohosApp->GetAssetDir().c_str());
    printf("[runner] files     : %s\n", Test::g_ohosApp->GetFilesDir().c_str());
    printf("[runner] cache     : %s\n", Test::g_ohosApp->GetCacheDir().c_str());
    fflush(stdout);
    const int result = Test::RunGoogleTest(argc, argv);

    // Leave via _exit() rather than returning.
    //
    // Once libEGL has been loaded, the platform's own atexit chain faults on the
    // way out: libEGL's EglWrapperLayer destructor dlclose()s libappexecfwk_core,
    // whose __cxa_finalize then dies in a static std::u16string destructor. The
    // whole stack is inside /system libraries -- no frame belongs to the engine,
    // the plugins or the tests -- and it happens strictly after RUN_ALL_TESTS()
    // has finished and gtest has flushed its XML report. Returning normally would
    // turn a fully passing run into exit code 139.
    fflush(nullptr);
    _exit(result);
}
