/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <meta/api/container/container_observer.h>
#include <meta/api/make_callback.h>
#include <meta/base/ref_uri.h>

#include "test_utils.h"
#include "testing_objects.h"
#include "util.h"

using namespace ::testing;

META_BEGIN_NAMESPACE()

namespace Test {

class UnitTest_ContainerObserverTest : public ::testing::Test {
    void SetUp() override
    {
        container_ = interface_pointer_cast<IContainer>(CreateTestContainer("Base"));
        ASSERT_NE(container_, nullptr);
        ASSERT_NE(observer_.GetIObject(), nullptr);
        container_->Add(CreateTestType<IObject>("Object1_1"));
        container_->Add(CreateTestType<IObject>("SameNameDifferentUid"));
        container_->Add(CreateTestType<IObject>("ObjectDupe"));
        container1_1_ = interface_pointer_cast<IContainer>(CreateTestContainer("Container1_1"));
        container_->Add(interface_pointer_cast<IObject>(container1_1_));
        container1_1_->Add(CreateTestType<IObject>("Object2_1"));
        container1_1_->Add(CreateTestType<IObject>("ObjectDupe"));
        container1_1_->Add(CreateTestType<IObject>("SameNameDifferentUid"));
        container_->Add(CreateTestType<IObject>("Object1_3"));
        container2_1_ = interface_pointer_cast<IContainer>(CreateTestContainer("SameNameDifferentUid"));
        container1_1_->Add(interface_pointer_cast<IObject>(container2_1_));

        observer_.Container(container_).OnDescendantChanged([this](const ChildChangedInfo& info) {
            if (info.type == ContainerChangeType::ADDED) {
                OnAdded(info);
            } else if (info.type == ContainerChangeType::REMOVED) {
                OnRemoved(info);
            } else if (info.type == ContainerChangeType::MOVED) {
                OnMoved(info);
            }
        });
    }
    void TearDown() override
    {
        container_.reset();
        observer_.ResetIObject();
    }

protected:
    void OnAdded(const ChildChangedInfo& info)
    {
        addedCalls_.push_back(info);
    }
    void OnRemoved(const ChildChangedInfo& info)
    {
        removedCalls_.push_back(info);
    }
    void OnMoved(const ChildChangedInfo& info)
    {
        movedCalls_.push_back(info);
    }

    BASE_NS::vector<ChildChangedInfo> addedCalls_;
    BASE_NS::vector<ChildChangedInfo> removedCalls_;
    BASE_NS::vector<ChildChangedInfo> movedCalls_;

    IContainer::Ptr container_;
    IContainer::Ptr container1_1_;
    IContainer::Ptr container2_1_;
    META_NS::ContainerObserver observer_;
};

TEST_F(UnitTest_ContainerObserverTest, AddDirectChild)
{
    auto child = CreateTestType<IObject>("DirectChild");
    ASSERT_TRUE(container_->Add(child));

    ASSERT_THAT(addedCalls_, SizeIs(1));
    ASSERT_THAT(removedCalls_, SizeIs(0));
    ASSERT_THAT(movedCalls_, SizeIs(0));

    auto info = addedCalls_[0];
    EXPECT_EQ(info.object, child);
    EXPECT_EQ(info.parent.lock(), container_);
}

TEST_F(UnitTest_ContainerObserverTest, AddDescendantChild)
{
    auto child = CreateTestType<IObject>("Descendant");
    ASSERT_TRUE(container2_1_->Add(child));

    ASSERT_THAT(addedCalls_, SizeIs(1));
    ASSERT_THAT(removedCalls_, SizeIs(0));
    ASSERT_THAT(movedCalls_, SizeIs(0));

    auto info = addedCalls_[0];
    EXPECT_EQ(info.object, child);
    EXPECT_EQ(info.parent.lock(), container2_1_);
}

TEST_F(UnitTest_ContainerObserverTest, ChangeAfterChild)
{
    auto container = CreateTestContainer<IContainer>("DescendantContainer");
    ASSERT_TRUE(container2_1_->Add(container));

    ASSERT_THAT(addedCalls_, SizeIs(1));
    ASSERT_THAT(removedCalls_, SizeIs(0));
    ASSERT_THAT(movedCalls_, SizeIs(0));

    auto info = addedCalls_[0];
    EXPECT_EQ(info.object, interface_pointer_cast<IObject>(container));
    EXPECT_EQ(info.parent.lock(), container2_1_);

    // Add a child to the newly-added descendant
    auto child = CreateTestType<IObject>("DescendantChild");
    ASSERT_TRUE(container->Add(child));
    ASSERT_THAT(addedCalls_, SizeIs(2));
    ASSERT_THAT(removedCalls_, SizeIs(0));
    ASSERT_THAT(movedCalls_, SizeIs(0));

    info = addedCalls_[1];
    EXPECT_EQ(info.object, child);
    EXPECT_EQ(info.parent.lock(), container);

    // Remove an ancestor of our manually added container hierarchy -> notification
    EXPECT_TRUE(container1_1_->Remove(container2_1_));
    // Remove a descendant of the ancestor we removed -> no notification
    EXPECT_TRUE(container->Remove(child));
    ASSERT_THAT(addedCalls_, SizeIs(2));
    ASSERT_THAT(removedCalls_, SizeIs(1));
    ASSERT_THAT(movedCalls_, SizeIs(0));

    info = removedCalls_[0];
    EXPECT_EQ(info.object, interface_pointer_cast<IObject>(container2_1_));
    EXPECT_EQ(info.parent.lock(), container1_1_);
}

TEST_F(UnitTest_ContainerObserverTest, RemoveDescendant)
{
    container_->Remove(container1_1_);
    ASSERT_THAT(addedCalls_, SizeIs(0));
    ASSERT_THAT(removedCalls_, SizeIs(1));
    ASSERT_THAT(movedCalls_, SizeIs(0));

    auto info = removedCalls_[0];
    EXPECT_EQ(info.object, interface_pointer_cast<IObject>(container1_1_));
    EXPECT_EQ(info.parent.lock(), container_);
    removedCalls_.clear();

    // Add a new child to container2_1_, we should not get notification
    // as container2_1_ is not part of container_'s child hierarchy anymore
    auto child = CreateTestType<IObject>("Descendant");
    container1_1_->Add(child);

    ASSERT_THAT(addedCalls_, SizeIs(0));
    ASSERT_THAT(removedCalls_, SizeIs(0));
    ASSERT_THAT(movedCalls_, SizeIs(0));
}

} // namespace Test
META_END_NAMESPACE()
