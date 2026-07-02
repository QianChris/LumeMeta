/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef META_INTERFACE_DETAIL_ANY_POINTER_COMPATIBILITY_H
#define META_INTERFACE_DETAIL_ANY_POINTER_COMPATIBILITY_H

#include <core/plugin/intf_interface.h>

#include <meta/base/interface_traits.h>
#include <meta/interface/intf_any.h>

META_BEGIN_NAMESPACE()

// NOLINTBEGIN(readability-identifier-naming)
using SharedPtrIInterface = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using SharedPtrConstIInterface = BASE_NS::shared_ptr<const CORE_NS::IInterface>;
constexpr TypeId SharedPtrIInterfaceId = UidFromType<SharedPtrIInterface>();
constexpr TypeId SharedPtrConstIInterfaceId = UidFromType<SharedPtrConstIInterface>();
using WeakPtrIInterface = BASE_NS::weak_ptr<CORE_NS::IInterface>;
using WeakPtrConstIInterface = BASE_NS::weak_ptr<const CORE_NS::IInterface>;
constexpr TypeId WeakPtrIInterfaceId = UidFromType<WeakPtrIInterface>();
constexpr TypeId WeakPtrConstIInterfaceId = UidFromType<WeakPtrConstIInterface>();
// NOLINTEND(readability-identifier-naming)

META_END_NAMESPACE()

#endif
