/**
 *  Copyright (C) 2023 Huawei Technologies Co, Ltd.
 */
#include <tester.h>

#include <gtest/gtest.h>

#include <meta/api/threading/mutex.h>

#include "testing_objects.h"

using namespace testing;

META_BEGIN_NAMESPACE()

namespace Test {

using CORE_NS::Mutex;
using CORE_NS::UniqueLock;

TEST(MutexTest, UniqueLock)
{
    Mutex m;
    {
        UniqueLock l { m };
        EXPECT_TRUE(l);
        l.Unlock();
        EXPECT_FALSE(l);
    }
    {
        UniqueLock l { m };
        EXPECT_TRUE(l);
        l.Unlock();
        EXPECT_FALSE(l);
        l.Lock();
        EXPECT_TRUE(l);
    }
    {
        UniqueLock l { m };
        EXPECT_TRUE(l);
        UniqueLock l2 = BASE_NS::move(l);
        EXPECT_FALSE(l);
        EXPECT_TRUE(l2);
        l = std::move(l2);
        EXPECT_FALSE(l2);
        EXPECT_TRUE(l);
    }
}

} // namespace Test

META_END_NAMESPACE()
