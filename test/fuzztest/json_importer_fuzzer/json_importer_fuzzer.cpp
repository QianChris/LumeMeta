/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "json_importer_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <base/containers/string_view.h>
#include <core/io/intf_file.h>
#include <core/os/platform_create_info.h>
#include <core/plugin/intf_plugin_register.h>
#include <meta/base/plugin.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/serialization/intf_importer.h>

#include "load_lib.h"

// The LumeMeta serialization deserializer (JsonInput) builds meta BaseObjects whose lifecycle calls
// GetObjectRegistry(), so it can only run with the meta object registry live. There is no headless
// path: we bring up the engine plugin registry + load the MetaObject plugin exactly like the meta
// test runner, then drive JsonImporter::ImportAsTree (parse JSON -> SerNode tree) over fuzz bytes.
// On-device only (needs libAGPDLL.z.so + the meta plugin on disk). Covers json_input.cpp +
// json_importer.cpp's tree-building path.

// In CORE_DYNAMIC builds these engine entry points are function pointers populated by dlsym from
// libAGPDLL.z.so. They must be defined in exactly one TU (mirrors the meta test runner).
CORE_BEGIN_NAMESPACE()
IPluginRegister& (*GetPluginRegister)(){nullptr};
void (*CreatePluginRegistry)(const struct PlatformCreateInfo& platformCreateInfo){nullptr};
CORE_END_NAMESPACE()

namespace {
// Read-only in-memory IFile over the fuzz bytes (stack-owned; Destroy is a no-op).
class MemoryFile final : public CORE_NS::IFile {
public:
    MemoryFile(const uint8_t* data, uint64_t size) : data_(data), size_(size)
    {}

    Mode GetMode() const override
    {
        return Mode::READ_ONLY;
    }
    void Close() override
    {
        pos_ = 0;
    }
    uint64_t Read(void* buffer, uint64_t count) override
    {
        if (buffer == nullptr || count == 0 || pos_ >= size_) {
            return 0;
        }
        const uint64_t avail = size_ - pos_;
        const uint64_t n = count < avail ? count : avail;
        std::memcpy(buffer, data_ + pos_, static_cast<size_t>(n));
        pos_ += n;
        return n;
    }
    uint64_t Write(const void*, uint64_t) override
    {
        return 0;
    }
    uint64_t Append(const void*, uint64_t, uint64_t) override
    {
        return 0;
    }
    uint64_t GetLength() const override
    {
        return size_;
    }
    bool Seek(uint64_t offset) override
    {
        if (offset > size_) {
            return false;
        }
        pos_ = offset;
        return true;
    }
    uint64_t GetPosition() const override
    {
        return pos_;
    }

private:
    void Destroy() override
    {}
    const uint8_t* data_;
    uint64_t size_;
    uint64_t pos_ = 0;
};

bool g_ready = false;
Test::DynamicLibrary g_engineLib;

// Brings up the engine plugin registry and loads the MetaObject plugin (the meta unit-test
// bootstrap). NOTE: the meta code then runs inside the loaded .so, which is NOT built with
// SanitizerCoverage/ASan -- so libFuzzer gets no coverage feedback and only hard crashes
// (segfaults) are caught. This is a smoke driver for ImportAsTree, not a coverage-guided fuzzer.
// A coverage-guided + sanitized version would require compiling lume_metaobject_src into this
// target and initialising the registry in-process (see FUZZ_COVERAGE_PLAN.md, Phase 2 notes).
bool InitMeta()
{
    g_engineLib.Load(OHOS_PLATFORM_CORE_PATH "libAGPDLL.z.so");
    if (!g_engineLib.IsLoaded()) {
        return false;
    }
    if (!g_engineLib.LoadFunction<decltype(CORE_NS::CreatePluginRegistry)>(
            CORE_NS::CreatePluginRegistry, "_ZN4Core20CreatePluginRegistryERKNS_18PlatformCreateInfoE")) {
        return false;
    }
    if (!g_engineLib.LoadFunction<decltype(CORE_NS::GetPluginRegister)>(
            CORE_NS::GetPluginRegister, "_ZN4Core17GetPluginRegisterEv")) {
        return false;
    }

    CORE_NS::PlatformCreateInfo info{};
    info.coreRootPath = OHOS_PLATFORM_CORE_PATH;
    info.appPluginPath = OHOS_PLATFORM_PLUGINS_PATH;
    CORE_NS::CreatePluginRegistry(info);

    const BASE_NS::Uid uids[]{META_NS::META_OBJECT_PLUGIN_UID};
    return CORE_NS::GetPluginRegister().LoadPlugins(uids);
}
}  // namespace

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    g_ready = InitMeta();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (!g_ready || data == nullptr || size == 0) {
        return 0;
    }

    auto importer = META_NS::GetObjectRegistry().Create<META_NS::IFileImporter>(META_NS::ClassId::JsonImporter);
    if (importer) {
        MemoryFile file(data, static_cast<uint64_t>(size));
        (void)importer->ImportAsTree(file);
    }

    return 0;
}
