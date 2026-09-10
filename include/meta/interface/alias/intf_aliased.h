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

#ifndef META_INTERFACE_ALIAS_IALIASED_H
#define META_INTERFACE_ALIAS_IALIASED_H

#include <base/containers/string.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/interface_macros.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The IAliased interface is implemented by classes that follow a named alias: the object names
 *        an alias, and what the alias resolves to decides what the object holds.
 *
 * This is the form for an object with a single obvious aliasable aspect, where naming the alias is all
 * the object needs to say — an alias node follows an alias to the node template it shows, so a provider
 * that defines the alias decides what the node shows and can change it at runtime. Aliasing an
 * arbitrary property of an arbitrary object needs no interface: bind the property to the alias
 * directly (see IAliasRegistry).
 */
class IAliased : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IAliased, "f9d60b6a-cbe2-4d02-b77c-7ab37d2eea96")
public:
    /**
     * @brief Name of the alias this object follows. Changing it detaches from the previous alias and
     *        follows the new one; an empty name detaches. Valid to set before the alias is defined —
     *        the object holds nothing until it is.
     */
    META_PROPERTY(BASE_NS::string, Alias)
};

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IAliased)

#endif
