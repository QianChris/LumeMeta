/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#include <functional>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <base/containers/atomics.h>

#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()
namespace Test {

TEST(SpinLockTest, LockUnlock)
{
    BASE_NS::SpinLock l;
    bool value = false;

    l.Lock();
    std::thread t { [&] {
        l.Lock();
        value = true;
        l.Unlock();
    } };
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    l.Unlock();
    if (t.joinable()) {
        t.join();
    }
    EXPECT_EQ(value, true);
}

TEST(SpinLockTest, MultiLockUnlock)
{
    BASE_NS::SpinLock l;
    int value = 0;

    std::vector<std::thread> threads;
    for (int i = 0; i != 5; ++i) {
        threads.push_back(std::thread { [&] {
            int v = 0;
            while (v < 100) {
                l.Lock();
                v = ++value;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                l.Unlock();
            }
        } });
    }
    for (auto&& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    EXPECT_TRUE(value >= 100);
}

} // namespace Test
META_END_NAMESPACE()
