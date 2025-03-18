/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <meta/api/container/find_containers.h>
#include <meta/api/make_callback.h>
#include <meta/api/object.h>
#include <meta/base/ref_uri.h>
#include <meta/interface/intf_attachment.h>
#include <meta/interface/intf_container_query.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_required_interfaces.h>

using namespace ::testing;

META_BEGIN_NAMESPACE()

namespace Test {

TEST(ContainerInfoTest, Self)
{
    /*Object object;
    auto info = interface_pointer_cast<IContainerQuery>(object);
    ASSERT_NE(info, nullptr);
    EXPECT_THAT(info->FindAllContainers(), SizeIs(1)); // attachments container
    */
    Object object;
    EXPECT_THAT(FindAllContainers(object), SizeIs(1)); // attachments container
}

// TEST(ContainerInfoTest, Container)
//{
//     ParallelAnimation object;
//     auto info = interface_pointer_cast<IContainerQuery>(object);
//     ASSERT_NE(info, nullptr);
//     auto attach = interface_pointer_cast<IAttach>(object);
//     ASSERT_NE(attach, nullptr);
//     auto containers = info->FindAllContainers<IAnimation>();
//     EXPECT_THAT(containers, UnorderedElementsAre(object));
//     containers = info->FindAllContainers();
//     EXPECT_THAT(containers, UnorderedElementsAre(object, attach->GetAttachmentContainer()));
// }
//
// TEST(ContainerInfoTest, Attachments)
//{
//     auto animation = ParallelAnimation();
//     auto container = GetObjectRegistry().Create<IContainer>(ClassId::ObjectContainer);
//     auto object = Object(interface_pointer_cast<IObject>(container)).Attach(animation).Attach(PropertyAnimation());
//     auto info = interface_pointer_cast<IContainerQuery>(object);
//     ASSERT_NE(info, nullptr);
//     auto attach = interface_pointer_cast<IAttach>(object);
//     ASSERT_NE(attach, nullptr);
//     // Get all containers which are compatible with IAnimation (i.e. our ParallelAnimation)
//     auto containers = info->FindAllContainers<IAnimation>();
//     EXPECT_THAT(containers, UnorderedElementsAre(container, animation));
//     // Get all containers which are compatible with IAttachment (i.e. the AttachmentContainer within the object)
//     containers = info->FindAllContainers<IAttachment>();
//     EXPECT_THAT(containers, UnorderedElementsAre(container, attach->GetAttachmentContainer()));
//     // Should return all containers, i.e. the attachment container and all of the attachments
//     // which are themselves a container
//     containers = info->FindAllContainers();
//     EXPECT_THAT(containers, UnorderedElementsAre(container, animation, attach->GetAttachmentContainer()));
// }
//
// TEST(ContainerInfoTest, MultiInterface)
//{
//     auto animation = ParallelAnimation();
//     auto container = GetObjectRegistry().Create<IContainer>(ClassId::ObjectContainer);
//     auto req = interface_cast<IRequiredInterfaces>(container);
//     ASSERT_TRUE(req);
//     EXPECT_TRUE(req->SetRequiredInterfaces({ IAnimation::UID, IAttachment::UID }));
//     auto object = Object(interface_pointer_cast<IObject>(container)).Attach(animation).Attach(PropertyAnimation());
//     auto info = interface_pointer_cast<IContainerQuery>(object);
//     ASSERT_NE(info, nullptr);
//     auto attach = interface_pointer_cast<IAttach>(object);
//     // Only our own container matches the search criteria
//     auto containers = info->FindAllContainers({ IAnimation::UID, IAttachment::UID });
//     EXPECT_THAT(containers, UnorderedElementsAre(container));
// }
//
// TEST(ContainerInfoTest, Invalid)
//{
//     auto animation = ParallelAnimation();
//     auto container = GetObjectRegistry().Create<IContainer>(ClassId::ObjectContainer);
//     auto req = interface_cast<IRequiredInterfaces>(container);
//     ASSERT_TRUE(req);
//     EXPECT_TRUE(req->SetRequiredInterfaces({ IAttach::UID }));
//     auto object = Object(interface_pointer_cast<IObject>(container)).Attach(animation).Attach(PropertyAnimation());
//     auto info = interface_pointer_cast<IContainerQuery>(object);
//     ASSERT_NE(info, nullptr);
//     auto attach = interface_pointer_cast<IAttach>(object);
//     // No containers should match the criteria
//     auto containers = info->FindAllContainers({ IAnimation::UID, IAttachment::UID });
//     EXPECT_THAT(containers, IsEmpty());
// }
//
// TEST(ContainerInfoTest, MaxCount)
//{
//     auto animation = ParallelAnimation();
//     auto container = GetObjectRegistry().Create<IContainer>(ClassId::ObjectContainer);
//     auto req = interface_cast<IRequiredInterfaces>(container);
//     ASSERT_TRUE(req);
//     EXPECT_TRUE(req->SetRequiredInterfaces({ IAttach::UID }));
//     auto object = Object(interface_pointer_cast<IObject>(container)).Attach(animation).Attach(PropertyAnimation());
//     auto info = interface_pointer_cast<IContainerQuery>(object);
//     ASSERT_NE(info, nullptr);
//     auto containers = info->FindAllContainers({ {}, 1 });
//     EXPECT_THAT(containers, UnorderedElementsAre(container));
//     containers = info->FindAllContainers({ { IAnimation::UID }, 1 });
//     EXPECT_THAT(containers, UnorderedElementsAre(animation));
//     EXPECT_TRUE(req->SetRequiredInterfaces({ IAnimation::UID }));
//     containers = info->FindAllContainers({ { IAnimation::UID }, 1 });
//     EXPECT_THAT(containers, UnorderedElementsAre(container));
//     containers = info->FindAllContainers({ { IAnimation::UID }, 0 });
//     EXPECT_THAT(containers, UnorderedElementsAre(container, animation));
//     EXPECT_EQ(info->FindAnyContainer<IAnimation>(), container);
// }
} // namespace Test
META_END_NAMESPACE()
