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

// clang-format off
#include "engine_test_property.h"
// clang-format on

#include <test_framework.h>

#include <core/property/property_handle_util.h>

#include <meta/api/engine/util.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/engine/intf_engine_value_manager.h>
#include <meta/interface/intf_object_registry.h>

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

struct InvalidType {};
META_TYPE(InvalidType);

META_BEGIN_NAMESPACE()
namespace UTest {

class API_EngineValueTest : public ::testing::Test {};

UNIT_TEST_F(API_EngineValueTest, Values, testing::ext::TestSize.Level1)
{
    auto& data = GetObjectRegistry().GetEngineData();

    auto accesses = data.GetAllRegisteredValueAccess();
    EXPECT_FALSE(accesses.empty());

    auto verifyClone = [](const IAny::Ptr& any) {
        ASSERT_TRUE(any);
        EXPECT_TRUE(any->Clone());
        EXPECT_TRUE(any->Clone(true));
        EXPECT_TRUE(any->Clone(false));
        EXPECT_TRUE(any->Clone(AnyCloneOptions {}));
        AnyCloneOptions opt;
        opt.value = CloneValueType::COPY_VALUE;
        opt.role = TypeIdRole::ITEM;
        EXPECT_TRUE(any->Clone(opt));
        opt.role = TypeIdRole::ARRAY;
        EXPECT_TRUE(any->Clone(opt));
        opt.value = CloneValueType::DEFAULT_VALUE;
        EXPECT_TRUE(any->Clone(opt));
    };

    // Go through all registered CoreAny/CoreArrayAny types and verify methods
    for (auto&& access : accesses) {
        auto type = BASE_NS::string(access.name);
        auto v = data.GetInternalValueAccess(access);
        ASSERT_TRUE(v);
        CORE_NS::Property p;
        p.type = access;
        auto any = v->CreateAny(p);
        auto serAny = v->CreateSerializableAny();
        ASSERT_TRUE(any);
        ASSERT_TRUE(serAny);
        if (auto et = interface_cast<IEngineType>(any)) {
            EXPECT_EQ(et->GetTypeDecl(), access);
        }
        AnyCloneOptions opt;
        opt.value = CloneValueType::COPY_VALUE;
        opt.role = TypeIdRole::ITEM;
        verifyClone(any->Clone(opt));
        opt.role = TypeIdRole::ARRAY;
        verifyClone(any->Clone(opt));
    }
}

class PropertyHandle;

namespace Internal {
static PropertyHandle* CurrentTestingProperty {};
}

class PropertyHandle : public CORE_NS::IPropertyHandle {
public:
    PropertyHandle(const CORE_NS::PropertyTypeDecl& decl) : byteSize_(decl.byteSize)
    {
        data_.resize(byteSize_);
        api_.size = [](uintptr_t container) { return Internal::CurrentTestingProperty->size_; };
        api_.resize = [](uintptr_t container, size_t newSize) {
            auto& ph = *Internal::CurrentTestingProperty;
            ph.size_ = newSize;
        };
        api_.erase = [](uintptr_t container, size_t index) {
            // Doesn't do anything but update internal size value
            auto& ph = *Internal::CurrentTestingProperty;
            ph.size_--;
        };
        api_.insert = [](uintptr_t container, size_t index) {
            // Doesn't do anything, returns the first (and only) value
            auto& ph = *Internal::CurrentTestingProperty;
            ph.size_++;
            return reinterpret_cast<uintptr_t>(&ph.data_[0]);
        };
        api_.get = [](uintptr_t container, size_t index) {
            // Doesn't do anything but return the first (and only) value
            auto& ph = *Internal::CurrentTestingProperty;
            return reinterpret_cast<uintptr_t>(&ph.data_[0]);
        };
    }
    const CORE_NS::IPropertyApi* Owner() const override
    {
        return nullptr;
    }

    size_t Size() const override
    {
        return data_.size();
    }
    const void* RLock() const override
    {
        return &data_[0];
    };
    void RUnlock() const override {};
    void* WLock() override
    {
        return &data_[0];
    };
    void WUnlock() override {};

    CORE_NS::ContainerApi* GetContainerApi()
    {
        return &api_;
    }

    size_t GetDataIndex(size_t index)
    {
        return index * byteSize_;
    }

private:
    CORE_NS::ContainerApi api_;
    size_t byteSize_;
    size_t size_ { 1 };
    BASE_NS::vector<uint8_t> data_;
};

UNIT_TEST_F(API_EngineValueTest, Access, testing::ext::TestSize.Level1)
{
    auto& data = GetObjectRegistry().GetEngineData();

    auto accesses = data.GetAllRegisteredValueAccess();
    EXPECT_FALSE(accesses.empty());

    auto testAccess = [](const IEngineInternalValueAccess::Ptr& access, const EnginePropertyParams& params,
                          const IAny::Ptr& any) {
        ASSERT_TRUE(any);
        access->SyncFromEngine(params, *any);
        access->SyncToEngine(*any, params);

        auto invalid = Any<InvalidType> {};
        access->SyncFromEngine(params, invalid);
        access->SyncToEngine(invalid, params);
    };

    auto verifyAny = [&](const IEngineInternalValueAccess::Ptr& access, PropertyHandle& ph, CORE_NS::Property& property,
                         const IAny::Ptr& any) {
        auto* ca = ph.GetContainerApi();
        EnginePropertyParams params;
        params.property = property;
        // Invalid handle
        testAccess(access, params, any);
        // Set handle
        EnginePropertyHandle handle;
        handle.handle = &ph;
        params.handle = handle;
        testAccess(access, params, any);
        // Set container methods
        params.containerMethods = ca;
        testAccess(access, params, any);
        // Set size to 0
        ca->resize(0, 0);
        testAccess(access, params, any);
    };

    // Go through all registered InternalValueAccess types and verify methods
    for (auto&& decl : accesses) {
        auto type = BASE_NS::string(decl.name);
        auto access = data.GetInternalValueAccess(decl);
        ASSERT_TRUE(access);

        EXPECT_TRUE(access->IsCompatible(decl)) << type.c_str();
        EXPECT_FALSE(access->IsCompatible({})) << type.c_str();

        // Create a dummy property for calling IInternalValueAccess methods
        auto ph = BASE_NS::make_unique<PropertyHandle>(decl);
        Internal::CurrentTestingProperty = ph.get(); // Store PropertyHandle* for ContainerApi
        auto* ca = ph->GetContainerApi();
        CORE_NS::Property property;
        property.type = decl;
        property.metaData.containerMethods = ph->GetContainerApi();
        property.count = 0;
        property.size = decl.byteSize;
        property.offset = 0;
        property.flags = 0;
        ca->property = property;
        auto any = access->CreateAny(property);
        verifyAny(access, *ph, property, any);
        AnyCloneOptions opt;
        opt.value = CloneValueType::COPY_VALUE;
        opt.role = TypeIdRole::ITEM;
        verifyAny(access, *ph, property, any->Clone(opt));
        opt.role = TypeIdRole::ARRAY;
        verifyAny(access, *ph, property, any->Clone(opt));
        Internal::CurrentTestingProperty = nullptr;
    }
}

} // namespace UTest
META_END_NAMESPACE()
