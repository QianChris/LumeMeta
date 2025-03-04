/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#ifndef META_TEST_UTIL_HEADER
#define META_TEST_UTIL_HEADER

#include <base/containers/vector.h>
#include <core/log.h>

#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/construct_property.h>

META_BEGIN_NAMESPACE()
namespace Test {

template<typename T>
typename META_NS::Property<T> CreateProperty(BASE_NS::string_view name = "", T value = {})
{
    return ConstructProperty<T>(name, value);
}

} // namespace Test

META_END_NAMESPACE()

BASE_BEGIN_NAMESPACE()

// our vector does not have op==
template<typename T>
bool operator==(const BASE_NS::vector<T>& l, const BASE_NS::vector<T>& r)
{
    if (l.size() != r.size()) {
        return false;
    }
    auto it1 = l.begin();
    auto it2 = r.begin();
    for (; it1 != l.end(); ++it1, ++it2) {
        if constexpr (META_NS::HasEqualOperator_v<T>) {
            if (!(*it1 == *it2)) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

template<typename T>
bool operator!=(const BASE_NS::vector<T>& l, const BASE_NS::vector<T>& r)
{
    return !(l == r);
}

BASE_END_NAMESPACE()

#endif
