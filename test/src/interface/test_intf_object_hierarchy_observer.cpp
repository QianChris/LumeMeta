/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <meta/api/container/object_hierarchy_observer.h>
#include <meta/api/content_object.h>
#include <meta/api/make_callback.h>
#include <meta/base/ref_uri.h>

#include "serialisation_utils.h"
#include "test_utils.h"
#include "testing_objects.h"
#include "util.h"

using namespace ::testing;

META_BEGIN_NAMESPACE()

namespace Test {

size_t GetMatching(const BASE_NS::vector<HierarchyChangedInfo>& changedCalls, HierarchyChangeType change,
    HierarchyChangeObjectType objectType)
{
    auto added = 0;
    for (const auto& info : changedCalls) {
        if (info.change == change && info.objectType == objectType) {
            added++;
        }
    }
    return added;
}

MATCHER_P(AddingChildrenEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::ADDING, HierarchyChangeObjectType::CHILD) == expected;
}
MATCHER_P(RemovingChildrenEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVING, HierarchyChangeObjectType::CHILD) == expected;
}
MATCHER_P(AddedChildrenEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::ADDED, HierarchyChangeObjectType::CHILD) == expected;
}
MATCHER_P(RemovedChildrenEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVED, HierarchyChangeObjectType::CHILD) == expected;
}
MATCHER_P(MovedChildrenEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::MOVED, HierarchyChangeObjectType::CHILD) == expected;
}
MATCHER_P(AddingContentEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::ADDING, HierarchyChangeObjectType::CONTENT) == expected;
}
MATCHER_P(AddedContentEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::ADDED, HierarchyChangeObjectType::CONTENT) == expected;
}
MATCHER_P(RemovingContentEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVING, HierarchyChangeObjectType::CONTENT) == expected;
}
MATCHER_P(RemovedContentEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVED, HierarchyChangeObjectType::CONTENT) == expected;
}
MATCHER_P(AddingAttachmentsEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::ADDING, HierarchyChangeObjectType::ATTACHMENT) == expected;
}
MATCHER_P(RemovingAttachmentsEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVING, HierarchyChangeObjectType::ATTACHMENT) == expected;
}
MATCHER_P(AddedAttachmentsEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::ADDED, HierarchyChangeObjectType::ATTACHMENT) == expected;
}
MATCHER_P(RemovedAttachmentsEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVED, HierarchyChangeObjectType::ATTACHMENT) == expected;
}
MATCHER_P(MovedAttachmentsEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::MOVED, HierarchyChangeObjectType::ATTACHMENT) == expected;
}

MATCHER_P(ObjectEq, expected, "")
{
    return interface_pointer_cast<IObject>(arg) == interface_pointer_cast<IObject>(expected);
}
MATCHER(ObjectIsNull, "")
{
    return interface_pointer_cast<IObject>(arg) == nullptr;
}

class UnitTest_ObjectHierarchyObserverTest : public ::testing::Test {
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
        container2_1_->Add(contentObject_);
        container1_1_->Add(interface_pointer_cast<IObject>(container2_1_));

        observer_.Target(interface_pointer_cast<IObject>(container_))
            .OnHierarchyChanged([this](const HierarchyChangedInfo& info) { OnHierarchyChanged(info); });
    }
    void TearDown() override
    {
        container_.reset();
        observer_.ResetIObject();
    }

public:
    struct ExpectedCallCount {
        size_t addingChildren {};
        size_t addedChildren {};
        size_t removingChildren {};
        size_t removedChildren {};
        size_t movedChildren {};
        size_t addingContent {};
        size_t addedContent {};
        size_t removingContent {};
        size_t removedContent {};
        size_t addingAttachments {};
        size_t addedAttachments {};
        size_t removingAttachments {};
        size_t removedAttachments {};
        size_t movedAttachments {};

        void ExpectAddChildren(size_t count = 1)
        {
            addingChildren += count;
            addedChildren += count;
        }
        void ExpectAddAttachments(size_t count = 1)
        {
            addingAttachments += count;
            addedAttachments += count;
        }
        void ExpectAddContent(size_t count = 1)
        {
            addingContent += count;
            addedContent += count;
        }
        void ExpectRemoveChildren(size_t count = 1)
        {
            removingChildren += count;
            removedChildren += count;
        }
        void ExpectRemoveAttachments(size_t count = 1)
        {
            removingAttachments += count;
            removedAttachments += count;
        }
        void ExpectRemoveContent(size_t count = 1)
        {
            removingContent += count;
            removedContent += count;
        }
        size_t Sum() const
        {
            return addingChildren + addedChildren + removingChildren + removedChildren + movedChildren + addingContent +
                   addedContent + removingContent + removedContent + addingAttachments + addedAttachments +
                   removingAttachments + removedAttachments + movedAttachments;
        }
    };

    void VerifyCalls(const ExpectedCallCount& expected)
    {
        EXPECT_THAT(changedCalls_, AddingChildrenEq(expected.addingChildren));
        EXPECT_THAT(changedCalls_, AddedChildrenEq(expected.addedChildren));
        EXPECT_THAT(changedCalls_, RemovingChildrenEq(expected.removingChildren));
        EXPECT_THAT(changedCalls_, RemovedChildrenEq(expected.removedChildren));
        EXPECT_THAT(changedCalls_, MovedChildrenEq(expected.movedChildren));
        EXPECT_THAT(changedCalls_, AddingContentEq(expected.addingContent));
        EXPECT_THAT(changedCalls_, AddedContentEq(expected.addedContent));
        EXPECT_THAT(changedCalls_, RemovingContentEq(expected.removingContent));
        EXPECT_THAT(changedCalls_, RemovedContentEq(expected.removedContent));
        EXPECT_THAT(changedCalls_, AddingAttachmentsEq(expected.addingAttachments));
        EXPECT_THAT(changedCalls_, AddedAttachmentsEq(expected.addedAttachments));
        EXPECT_THAT(changedCalls_, RemovingAttachmentsEq(expected.removingAttachments));
        EXPECT_THAT(changedCalls_, RemovedAttachmentsEq(expected.removedAttachments));
        EXPECT_THAT(changedCalls_, MovedAttachmentsEq(expected.movedAttachments));
        ASSERT_EQ(changedCalls_.size(), expected.Sum());
    }
    void ResetCalls()
    {
        changedCalls_.clear();
    }

protected:
    void OnHierarchyChanged(const HierarchyChangedInfo& info)
    {
        changedCalls_.push_back(info);
    }

    BASE_NS::vector<HierarchyChangedInfo> changedCalls_;

    IContainer::Ptr container_;
    IContainer::Ptr container1_1_;
    IContainer::Ptr container2_1_;
    META_NS::ContentObject contentObject_;
    META_NS::ObjectHierarchyObserver observer_;
};

TEST_F(UnitTest_ObjectHierarchyObserverTest, AddDirectChild)
{
    auto child = CreateTestType<IObject>("DirectChild");
    ASSERT_TRUE(container_->Add(child));

    ExpectedCallCount expected;
    expected.ExpectAddChildren();
    VerifyCalls(expected);
    ASSERT_TRUE(!changedCalls_.empty());
    auto& info = changedCalls_[0];
    EXPECT_EQ(info.object, child);
    EXPECT_THAT(info.parent.lock(), ObjectEq(container_));
}

TEST_F(UnitTest_ObjectHierarchyObserverTest, AddDescendantChild)
{
    auto child = CreateTestType<IObject>("DirectChild");
    ASSERT_TRUE(container2_1_->Add(child));

    ExpectedCallCount expected;
    expected.ExpectAddChildren();
    VerifyCalls(expected);
    ASSERT_TRUE(!changedCalls_.empty());
    auto& info = changedCalls_[0];
    EXPECT_EQ(info.object, child);
    EXPECT_THAT(info.parent.lock(), ObjectEq(container2_1_));
}

TEST_F(UnitTest_ObjectHierarchyObserverTest, AddDescendantContent)
{
    auto child = CreateTestType<IObject>("Content");
    auto child2 = CreateTestType<IObject>("Content2");
    {
        // Set content
        contentObject_.SetContent(child);
        ExpectedCallCount expected;
        expected.ExpectAddContent();
        VerifyCalls(expected);
        ASSERT_TRUE(!changedCalls_.empty());
        auto& info = changedCalls_[0];
        EXPECT_EQ(info.object, child);
        EXPECT_THAT(info.parent.lock(), ObjectEq(contentObject_));
    }
    ResetCalls();
    {
        // Change content
        contentObject_.SetContent(child2);
        ExpectedCallCount expected;
        expected.ExpectAddContent();
        expected.ExpectRemoveContent();
        VerifyCalls(expected);
        ASSERT_TRUE(changedCalls_.size() > 2);
        auto& removed = changedCalls_[0];
        auto& added = changedCalls_[2];
        EXPECT_EQ(removed.object, child);
        EXPECT_THAT(removed.parent.lock(), ObjectEq(contentObject_));
        EXPECT_EQ(added.object, child2);
        EXPECT_THAT(added.parent.lock(), ObjectEq(contentObject_));
    }
    ResetCalls();
    {
        // Remove content
        contentObject_.SetContent(nullptr);
        ExpectedCallCount expected;
        expected.ExpectRemoveContent();
        VerifyCalls(expected);
        ASSERT_TRUE(!changedCalls_.empty());
        auto& removed = changedCalls_[0];
        EXPECT_EQ(removed.object, child2);
        EXPECT_THAT(removed.parent.lock(), ObjectEq(contentObject_));
    }
}

TEST_F(UnitTest_ObjectHierarchyObserverTest, ContentChangeAfterRemove)
{
    auto child = CreateTestType<IObject>("Content");
    {
        contentObject_.SetContent(child);
        ExpectedCallCount expected;
        expected.ExpectAddContent();
        VerifyCalls(expected);
        ASSERT_TRUE(!changedCalls_.empty());
        auto& info = changedCalls_[0];
        EXPECT_EQ(info.object, child);
        EXPECT_THAT(info.parent.lock(), ObjectEq(contentObject_));
    }
    ResetCalls();
    {
        EXPECT_TRUE(container2_1_->Remove(contentObject_));
        ExpectedCallCount expected;
        expected.ExpectRemoveChildren();
        VerifyCalls(expected);
        ASSERT_TRUE(!changedCalls_.empty());
        auto& info = changedCalls_[0];
        EXPECT_EQ(info.object, contentObject_);
        EXPECT_THAT(info.parent.lock(), ObjectEq(container2_1_));
    }
    ResetCalls();
    {
        // Not in hierarchy anymore so we should not get any events
        contentObject_.SetContent(nullptr);
        VerifyCalls({});
    }
}

TEST_F(UnitTest_ObjectHierarchyObserverTest, Attachment)
{
    auto& obr = GetObjectRegistry();
    auto attachment1 = obr.Create<IAttachment>(ClassId::TestAttachment);
    auto test1 = interface_cast<ITestAttachment>(attachment1);
    ASSERT_TRUE(test1);
    {
        contentObject_.Attach(attachment1);
        ExpectedCallCount expected;
        expected.ExpectAddAttachments();
        VerifyCalls(expected);
        EXPECT_EQ(test1->GetAttachCount(), 1);
        EXPECT_EQ(test1->GetDetachCount(), 0);
        EXPECT_THAT(contentObject_, ObjectEq(GetValue(attachment1->AttachedTo())));
    }
    ResetCalls();
    {
        contentObject_.Detach(attachment1);
        ExpectedCallCount expected;
        expected.ExpectRemoveAttachments();
        VerifyCalls(expected);
        EXPECT_EQ(test1->GetAttachCount(), 1);
        EXPECT_EQ(test1->GetDetachCount(), 1);
        EXPECT_THAT(GetValue(attachment1->AttachedTo()), ObjectIsNull());
    }
}

TEST_F(UnitTest_ObjectHierarchyObserverTest, ContainerDescendantHierarchy)
{
    auto container = CreateTestContainer<IContainer>("DescendamtHierarchy");
    container->Add(CreateTestType<IObject>("DescendamtChild"));
    auto childContainer = CreateTestContainer<IContainer>("DescendamtHierarchyChildContainer");
    container->Add(interface_pointer_cast<IObject>(childContainer));
    auto childOfChild = CreateTestType<IObject>("ChildOfChild");
    childContainer->Add(childOfChild);
    auto attachment = CreateTestAttachment<IAttachment>("ChildOfChildAttachment");
    auto attach = interface_cast<IAttach>(childOfChild);
    ASSERT_TRUE(attach);
    ASSERT_TRUE(attach->Attach(attachment));

    {
        EXPECT_TRUE(container2_1_->Add(interface_pointer_cast<IObject>(container)));
        ExpectedCallCount expected;
        expected.ExpectAddChildren();
        VerifyCalls(expected);
    }
    ResetCalls();
    {
        EXPECT_TRUE(attach->Detach(attachment));
        ExpectedCallCount expected;
        expected.ExpectRemoveAttachments();
        VerifyCalls(expected);
    }
    ResetCalls();
    {
        EXPECT_TRUE(attach->Attach(attachment));
        ExpectedCallCount expected;
        expected.ExpectAddAttachments();
        VerifyCalls(expected);
    }
    ResetCalls();
    {
        EXPECT_TRUE(container->Remove(interface_pointer_cast<IObject>(childContainer)));
        ExpectedCallCount expected;
        expected.ExpectRemoveChildren();
        VerifyCalls(expected);
        EXPECT_TRUE(childContainer->Add(CreateTestType("AddedLater")));
        VerifyCalls(expected);
        EXPECT_TRUE(container->Add(interface_pointer_cast<IObject>(childContainer)));
        expected.ExpectAddChildren();
        VerifyCalls(expected);
    }
    ResetCalls();
    {
        EXPECT_TRUE(container2_1_->Remove(interface_pointer_cast<IObject>(container)));
        ExpectedCallCount expected;
        expected.ExpectRemoveChildren();
        VerifyCalls(expected);
    }
    ResetCalls();
    {
        EXPECT_TRUE(attach->Detach(attachment));
        VerifyCalls({});
    }
}

TEST(ObjectHierarchyObserverTest, Properties)
{
    auto obj = CreateTestType();
    auto att = interface_cast<IAttach>(obj);
    ASSERT_TRUE(att);

    ObjectHierarchyObserver obs;
    HierarchyChangedInfo info;

    obs.Target(interface_pointer_cast<IObject>(att->GetAttachmentContainer(true)),
        HierarchyChangeModeValue(HierarchyChangeMode::NOTIFY_CONTAINER) | HierarchyChangeMode::NOTIFY_OBJECT);
    obs.OnHierarchyChanged([&info](const HierarchyChangedInfo& i) { info = i; });

    obj->First()->SetValue(2);
    EXPECT_EQ(interface_pointer_cast<IProperty>(info.object), obj->First().GetProperty());
    EXPECT_EQ(info.change, HierarchyChangeType::CHANGED);
    EXPECT_EQ(info.objectType, HierarchyChangeObjectType::CHILD);
    EXPECT_EQ(info.parent.lock(), interface_pointer_cast<IObject>(att->GetAttachmentContainer(true)));

    obj->Second()->SetValue("hips");
    EXPECT_EQ(interface_pointer_cast<IProperty>(info.object), obj->Second().GetProperty());
}

TEST(ObjectHierarchyObserverTest, GetAll)
{
    ObjectHierarchyObserver obs;
    EXPECT_THAT(obs.GetAllObserved(), IsEmpty());

    auto containerObject = interface_pointer_cast<IObject>(CreateTestContainer("Container"));
    auto childObject = interface_pointer_cast<IObject>(CreateTestType("TestType"));

    auto container = interface_pointer_cast<IContainer>(containerObject);
    container->Add(childObject);

    EXPECT_TRUE(obs.Target(containerObject));
    EXPECT_THAT(obs.GetAllObserved(), UnorderedElementsAre(containerObject, childObject));
    EXPECT_THAT(obs.GetAllObserved<IContainer>(), UnorderedElementsAre(container));

    container->Remove(childObject);
    EXPECT_THAT(obs.GetAllObserved(), UnorderedElementsAre(containerObject));
}

TEST(ObjectHierarchyObserverTest, Remove)
{
    BASE_NS::vector<IObject::Ptr> removed;
    ObjectHierarchyObserver obs;
    auto callCount = 0;

    obs.OnHierarchyChanged([&removed, &callCount](const HierarchyChangedInfo& info) {
        if (info.change == HierarchyChangeType::REMOVED) {
            callCount++;
            removed.push_back(info.object);
        }
    });

    auto containerObject = interface_pointer_cast<IObject>(CreateTestContainer("Container"));
    auto childContainerObject = interface_pointer_cast<IObject>(CreateTestContainer("Container"));
    auto childObject = interface_pointer_cast<IObject>(CreateTestType("TestType"));
    {
        auto container = interface_pointer_cast<IContainer>(containerObject);
        auto childContainer = interface_pointer_cast<IContainer>(childContainerObject);
        EXPECT_TRUE(container->Add(childContainerObject));
        EXPECT_TRUE(childContainer->Add(childObject));
        EXPECT_TRUE(obs.Target(containerObject));
        EXPECT_THAT(obs.GetAllObserved(), UnorderedElementsAre(containerObject, childContainerObject, childObject));

        container->RemoveAll();
    }

    EXPECT_EQ(callCount, 1);
    EXPECT_THAT(removed, ElementsAre(childContainerObject));
}

TEST(ObjectHierarchyObserverTest, DestroyEvents)
{
    BASE_NS::vector<IObject::Ptr> removed;
    ObjectHierarchyObserver obs;
    auto callCount = 0;
    auto removingCallCount = 0;

    auto containerObject = interface_pointer_cast<IObject>(CreateTestContainer("Container"));
    auto childContainerObject = interface_pointer_cast<IObject>(CreateTestContainer("Container"));
    auto childObject = interface_pointer_cast<IObject>(CreateTestType("TestType"));
    auto childObject2 = interface_pointer_cast<IObject>(CreateTestType("TestType"));
    auto childObject3 = interface_pointer_cast<IObject>(CreateTestType("TestType"));
    auto childObject4 = interface_pointer_cast<IObject>(CreateTestType("TestType"));
    {
        auto container = interface_pointer_cast<IContainer>(containerObject);
        auto childContainer = interface_pointer_cast<IContainer>(childContainerObject);
        EXPECT_TRUE(container->Add(childContainerObject));
        EXPECT_TRUE(container->Add(childObject2));
        EXPECT_TRUE(container->Add(childObject3));
        EXPECT_TRUE(container->Insert(1, childObject4));
        EXPECT_TRUE(childContainer->Add(childObject));
        EXPECT_TRUE(obs.Target(containerObject));
        EXPECT_THAT(obs.GetAllObserved(), UnorderedElementsAre(containerObject, childContainerObject, childObject,
                                              childObject4, childObject2, childObject3));
        container->Remove(childObject3);
        EXPECT_THAT(obs.GetAllObserved(),
            UnorderedElementsAre(containerObject, childContainerObject, childObject, childObject4, childObject2));
    }

    obs.OnHierarchyChanged([&](const HierarchyChangedInfo& info) {
        if (info.objectType == HierarchyChangeObjectType::CHILD) {
            if (info.change == HierarchyChangeType::REMOVED) {
                callCount++;
                removed.push_back(info.object);
            }
            if (info.change == HierarchyChangeType::REMOVING) {
                ++removingCallCount;
            }
        }
    });

    containerObject.reset();
    EXPECT_EQ(callCount, 3);
    EXPECT_EQ(removingCallCount, 3);
    EXPECT_THAT(removed, ElementsAre(childContainerObject, childObject4, childObject2));
}

TEST(ObjectHierarchyObserverTest, DestroyEventsContent)
{
    BASE_NS::vector<IObject::Ptr> removed;
    ObjectHierarchyObserver obs;
    auto callCount = 0;

    obs.OnHierarchyChanged([&removed, &callCount](const HierarchyChangedInfo& info) {
        if (info.objectType == HierarchyChangeObjectType::CONTENT) {
            if (info.change == HierarchyChangeType::REMOVED) {
                callCount++;
                removed.push_back(info.object);
            }
        }
    });

    auto contentObject = META_NS::GetObjectRegistry().Create(ClassId::ContentObject);
    auto childObject = interface_pointer_cast<IObject>(CreateTestType("TestType"));
    {
        auto content = interface_pointer_cast<IContent>(contentObject);
        EXPECT_TRUE(content->SetContent(childObject));
        EXPECT_TRUE(obs.Target(contentObject));
        EXPECT_THAT(obs.GetAllObserved(), UnorderedElementsAre(contentObject, childObject));
    }

    contentObject.reset();
    EXPECT_EQ(callCount, 1);
    EXPECT_THAT(removed, ElementsAre(childObject));
}

TEST(ObjectHierarchyObserverTest, DoNotSerialise)
{
    TestSerialiser ser;
    {
        ObjectHierarchyObserver obs;
        auto containerObject = interface_pointer_cast<IObject>(CreateTestContainer("Container"));
        EXPECT_TRUE(obs.Target(containerObject));
        EXPECT_THAT(obs.GetAllObserved(), UnorderedElementsAre(containerObject));
        auto i = interface_cast<IAttach>(containerObject);
        ASSERT_TRUE(i);
        EXPECT_EQ(i->GetAttachments<IObjectHierarchyObserver>().size(), 1);
        ser.Export(containerObject);
    }

    auto obj = ser.Import<IAttach>();
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->GetAttachments<IObjectHierarchyObserver>().size(), 0);
}

} // namespace Test
META_END_NAMESPACE()
