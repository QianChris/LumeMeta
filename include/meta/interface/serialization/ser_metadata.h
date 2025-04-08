/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: Serialisation metadata
 * Author: Mikael Kilpeläinen
 */

#ifndef META_INTERFACE_SERIALIZATION_SER_METADATA_H
#define META_INTERFACE_SERIALIZATION_SER_METADATA_H

#include <meta/base/namespace.h>
#include <meta/base/version.h>

META_BEGIN_NAMESPACE()

struct SerMetadataEntity {
    BASE_NS::string key;
    BASE_NS::string data;
};
using SerMetadata = BASE_NS::vector<SerMetadataEntity>;

struct SerMetadataValues : SerMetadata {
    SerMetadataValues() = default;
    SerMetadataValues(SerMetadata m) : SerMetadata(BASE_NS::move(m)) {}

    SerMetadataValues& SetVersion(const Version& ver)
    {
        for (auto&& v : *this) {
            if (v.key == "version") {
                v.data = ver.ToString();
                return *this;
            }
        }
        push_back({ "version", ver.ToString() });
        return *this;
    }
    Version GetVersion() const
    {
        for (auto&& v : *this) {
            if (v.key == "version") {
                return Version(v.data);
            }
        }
        return {};
    }
    SerMetadataValues& SetType(BASE_NS::string_view type)
    {
        for (auto&& v : *this) {
            if (v.key == "type") {
                v.data = type;
                return *this;
            }
        }
        push_back({ "type", BASE_NS::string(type) });
        return *this;
    }
    BASE_NS::string GetType() const
    {
        for (auto&& v : *this) {
            if (v.key == "type") {
                return v.data;
            }
        }
        return {};
    }
};

META_END_NAMESPACE()

#endif
