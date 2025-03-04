/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */

#ifndef META_TEST_TEST_DATA_HELPERS_H
#define META_TEST_TEST_DATA_HELPERS_H

#include <tuple>
#include <type_traits>

#include <gtest/gtest.h>

#include <base/containers/type_traits.h>
#include <base/containers/vector.h>

namespace META_NS {
namespace Test {

template<typename T>
struct TType {
    using Type = T;
    BASE_NS::vector<std::function<Type()>> values;

    template<typename... Args, typename = BASE_NS::enable_if_t<std::conjunction_v<std::is_convertible<Args, Type>...>>>
    TType(Args... args) : values { [args] { return static_cast<T>(args); }... }
    {}

    TType(std::initializer_list<std::function<Type()>> values) : values { values } {}
};

template<typename... T>
struct TestTypes {
    using Types = ::testing::Types<T...>;
    using Tuple = std::tuple<TType<T>...>;

    TestTypes(TType<T>... v) : values { v... } {}

    template<typename Type>
    auto GetValue(size_t index) const
    {
        auto v = std::get<TType<Type>>(values);
        CORE_ASSERT_MSG(index < v.values.size(), "invalid test values");
        return v.values[index]();
    }

    Tuple values;
};

} // namespace Test
} // namespace META_NS

#endif
