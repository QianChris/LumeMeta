/**
 *  Copyright (C) 2021-2023 Huawei Technologies Co, Ltd.
 */
#include <bitset>
#include <limits>
#include <tester.h>
#include <type_traits>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <base/util/uid_util.h>

#include <meta/api/make_callback.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_class_registry.h>
#include <meta/interface/intf_object_registry.h>

using namespace ::testing;

META_BEGIN_NAMESPACE()
namespace Test {

class UnitTest_ClassRegistryTest : public ::testing::Test {
protected:
    UnitTest_ClassRegistryTest() : classRegistry_(META_NS::GetClassRegistry()) {}

    void SetUp() override {}
    void TearDown() override {}

protected:
    IClassRegistry& classRegistry_;
};

MATCHER_P(ContainsClass, uid, "")
{
    for (const auto& info : arg) {
        if (info && info->GetClassInfo().Id() == uid) {
            return true;
        }
    }
    return false;
}

TEST_F(UnitTest_ClassRegistryTest, GetAllClasses)
{
    auto all = classRegistry_.GetAllTypes({});
    EXPECT_THAT(all, Not(IsEmpty()));
}

TEST_F(UnitTest_ClassRegistryTest, GetAllTypesUis)
{
    auto animations = classRegistry_.GetAllTypes({ IAnimation::UID });
    EXPECT_THAT(animations, Not(IsEmpty()));
    // EXPECT_THAT(animations, ContainsClass(ClassId::ParallelAnimation));
    // EXPECT_THAT(animations, ContainsClass(ClassId::SequentialAnimation));
    // EXPECT_THAT(animations, ContainsClass(ClassId::TrackAnimation));
    EXPECT_THAT(animations, ContainsClass(ClassId::KeyframeAnimation));
    // EXPECT_THAT(animations, ContainsClass(ClassId::PropertyAnimation));

    auto staggered = classRegistry_.GetAllTypes({ IStaggeredAnimation::UID });
    // EXPECT_THAT(staggered, ContainsClass(ClassId::ParallelAnimation));
    // EXPECT_THAT(staggered, ContainsClass(ClassId::SequentialAnimation));
    // EXPECT_THAT(staggered, Not(ContainsClass(ClassId::TrackAnimation)));
    EXPECT_THAT(staggered, Not(ContainsClass(ClassId::KeyframeAnimation)));
    // EXPECT_THAT(staggered, Not(ContainsClass(ClassId::PropertyAnimation)));
}

META_REGISTER_CLASS(MyObject1, "440d971e-afae-497a-95c3-97c3ba7f5d18", META_NS::ObjectCategoryBits::APPLICATION)
META_REGISTER_CLASS(MyObject2, "f8d418d7-99aa-4b19-92b7-319959cc98f1", META_NS::ObjectCategoryBits::APPLICATION)

class MyObject1 : public META_NS::ObjectFwd {
    META_OBJECT(MyObject1, ClassId::MyObject1, ObjectFwd, META_NS::ClassId::Object)
};
class MyObject2 : public META_NS::ObjectFwd {
    META_OBJECT(MyObject2, ClassId::MyObject2, ObjectFwd, META_NS::ClassId::Object)
};

TEST_F(UnitTest_ClassRegistryTest, Events)
{
    auto& objReg = META_NS::GetObjectRegistry();
    auto& classReg = objReg.GetClassRegistry();

    int registered = 0;
    int unregistered = 0;
    ClassRegistrationInfo lastRegistered;
    ClassRegistrationInfo lastUnregistered;

    auto token1 = classReg.OnClassRegistered()->AddHandler(META_NS::MakeCallback<META_NS::IOnClassRegistrationChanged>(
        [&registered, &lastRegistered](const ClassRegistrationInfo& info) {
            registered++;
            lastRegistered = info;
        }));

    auto token2 =
        classReg.OnClassUnregistered()->AddHandler(META_NS::MakeCallback<META_NS::IOnClassRegistrationChanged>(
            [&unregistered, &lastUnregistered](const ClassRegistrationInfo& info) {
                unregistered++;
                lastUnregistered = info;
            }));

    EXPECT_TRUE(objReg.RegisterObjectType(MyObject1::GetFactory()));
    EXPECT_EQ(registered, 1);
    EXPECT_EQ(unregistered, 0);
    EXPECT_EQ(lastRegistered.classInfo, MyObject1::GetFactory());
    EXPECT_EQ(lastUnregistered.classInfo, nullptr);
    lastRegistered.classInfo.reset();

    EXPECT_FALSE(objReg.RegisterObjectType(MyObject1::GetFactory()));
    EXPECT_EQ(registered, 1);
    EXPECT_EQ(unregistered, 0);
    EXPECT_EQ(lastRegistered.classInfo, nullptr);
    EXPECT_EQ(lastUnregistered.classInfo, nullptr);

    EXPECT_TRUE(objReg.RegisterObjectType(MyObject2::GetFactory()));
    EXPECT_EQ(registered, 2);
    EXPECT_EQ(unregistered, 0);
    EXPECT_EQ(lastRegistered.classInfo, MyObject2::GetFactory());
    EXPECT_EQ(lastUnregistered.classInfo, nullptr);
    lastRegistered.classInfo.reset();

    EXPECT_TRUE(objReg.UnregisterObjectType(MyObject1::GetFactory()));
    EXPECT_EQ(registered, 2);
    EXPECT_EQ(unregistered, 1);
    EXPECT_EQ(lastRegistered.classInfo, nullptr);
    EXPECT_EQ(lastUnregistered.classInfo, MyObject1::GetFactory());
    lastUnregistered.classInfo.reset();

    EXPECT_FALSE(objReg.UnregisterObjectType(MyObject1::GetFactory()));
    EXPECT_EQ(registered, 2);
    EXPECT_EQ(unregistered, 1);
    EXPECT_EQ(lastRegistered.classInfo, nullptr);
    EXPECT_EQ(lastUnregistered.classInfo, nullptr);

    EXPECT_TRUE(objReg.UnregisterObjectType(MyObject2::GetFactory()));
    EXPECT_EQ(registered, 2);
    EXPECT_EQ(unregistered, 2);
    EXPECT_EQ(lastRegistered.classInfo, nullptr);
    EXPECT_EQ(lastUnregistered.classInfo, MyObject2::GetFactory());

    EXPECT_TRUE(classReg.OnClassRegistered()->RemoveHandler(token1));
    EXPECT_TRUE(classReg.OnClassUnregistered()->RemoveHandler(token2));
}

} // namespace Test
META_END_NAMESPACE()
