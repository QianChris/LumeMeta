/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#include <tester.h>
#include <testing_objects.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/api/animation/animation.h>
#include <meta/api/object.h>
#include <meta/api/object_name.h>

#include "serialisation_utils.h"

using namespace testing;

META_BEGIN_NAMESPACE()

namespace Test {

TEST(ObjectTest, ObjectBaseAPI)
{
    META_NS::Object object;

    EXPECT_NE(object.GetIObject(), nullptr);
    EXPECT_NE(object.GetInterfacePtr<IObject>(), nullptr);
    EXPECT_EQ(object.GetInterfacePtr<IObject>(), object.GetIObject());
    EXPECT_EQ(interface_pointer_cast<IObject>(object), object);
    EXPECT_EQ(interface_cast<IObject>(object), object.GetIObject().get());

    META_NS::Object object2;
    ASSERT_TRUE(object2.Initialize(object));
    EXPECT_EQ(object, object2);
    EXPECT_TRUE(object == object2);
    EXPECT_FALSE(object != object2);

    META_NS::Object object3;
    EXPECT_TRUE(object != object3);
    EXPECT_FALSE(object == object3);
    EXPECT_FALSE(object3); // not initialized yet
    EXPECT_NE(object3.GetInterfacePtr<IObject>(), nullptr);
    EXPECT_TRUE(object3); // Initialized after get
}

TEST(ObjectTest, TypeConversion)
{
    META_NS::Object o1(interface_pointer_cast<IObject>(CreateTestType()));
    META_NS::Object o2(interface_pointer_cast<IObject>(CreateTestContainer()));

    EXPECT_TRUE(o1);
    EXPECT_TRUE(o2);
    EXPECT_NE(o1.GetIObject()->GetClassId(), o2.GetIObject()->GetClassId());

    o1 = o2; // Changes type
    EXPECT_TRUE(o1);
    EXPECT_EQ(o1.GetIObject(), o2.GetIObject());
    EXPECT_EQ(o1.GetIObject()->GetClassId(), o2.GetIObject()->GetClassId());
}
/*
TEST(ObjectTest, InvalidTypes)
{
    META_NS::Object o1(interface_pointer_cast<IObject>(CreateTestType()));
    auto a1 = META_NS::PropertyAnimation().Duration(TimeSpan::Milliseconds(100));

    EXPECT_TRUE(o1);
    EXPECT_TRUE(a1);
    EXPECT_NE(o1.GetIObject()->GetClassId(), a1.GetIObject()->GetClassId());

    auto a1Ptr = a1.GetInterfacePtr<META_NS::IAnimation>();
    ASSERT_NE(a1Ptr, nullptr);

    META_NS::Object& a1Obj = a1;
    auto classIdBefore = a1Obj.GetIObject()->GetClassId();
    EXPECT_FALSE(a1Obj.Initialize(o1)); // a1Obj Object& is PropertyAnimation, so we should not be able to initialize it
                                        // with an Object of different type
    auto classIdAfter = a1Obj.GetIObject()->GetClassId();
    EXPECT_EQ(classIdBefore, classIdAfter);
    EXPECT_NE(a1.GetIObject(), nullptr);

    static constexpr auto dur = TimeSpan::Milliseconds(42);
    auto anim = PropertyAnimation().Duration(dur); // Init an animation
    Object o(anim);                                // Store a reference in Object
    PropertyAnimation anim2;
    anim2 = o; // Initialize another (valid) animation with the same object
    EXPECT_EQ(anim2.TotalDuration().Get(), dur);
    EXPECT_EQ(anim, anim2);

    SequentialAnimation anim3(o); // Init with incompatible
    EXPECT_FALSE(anim3);
    EXPECT_NE(anim, anim3);

    SequentialAnimation anim4;
    anim4 = o; // Assign with incompatible
    EXPECT_FALSE(anim4);
    EXPECT_NE(anim, anim4);
}
*/
TEST(ObjectTest, LazyInitBase)
{
    static constexpr auto propName = "value";
    auto o1 = Object();
    Object o2;

    EXPECT_FALSE(o1);
    EXPECT_FALSE(o2);
    o2 = o1; // Does nothing
    EXPECT_TRUE(o1);
    EXPECT_TRUE(o2);
    Object o3;
    EXPECT_FALSE(o3);
    o1.MetaProperty(propName, 5);
    o3 = o1;
    EXPECT_TRUE(o1);
    EXPECT_TRUE(o2);
    EXPECT_TRUE(o3);
    EXPECT_EQ(o1, o2);
    EXPECT_EQ(o1, o3);

    auto p1 = o1.Metadata().GetProperty(propName);
    auto p2 = o2.Metadata().GetProperty(propName);
    auto p3 = o3.Metadata().GetProperty(propName);
    EXPECT_EQ(p1, p3);
    EXPECT_EQ(p2, p1);
}

/*
TEST(ObjectTest, LazyInitDerived)
{
    auto p1 = PropertyAnimation();
    PropertyAnimation p2;
    EXPECT_FALSE(p1);
    EXPECT_FALSE(p2);
    p2 = p1;
    EXPECT_TRUE(p1);
    EXPECT_TRUE(p2);
    EXPECT_EQ(p1, p2);

    PropertyAnimation p3;
    p1.Duration(TimeSpan::Milliseconds(42));
    p3 = p1;
    EXPECT_TRUE(p1);
    EXPECT_TRUE(p2);
    EXPECT_TRUE(p3);
    EXPECT_EQ(p1, p3);

    EXPECT_EQ(p1.TotalDuration().Get(), p2.TotalDuration().Get());
    EXPECT_EQ(p1.TotalDuration().Get(), p3.TotalDuration().Get());
}
*/
/*
TEST(ObjectTest, Initialize)
{
    static constexpr auto propName = "value";
    const int value = 32;
    auto p1 = PropertyAnimation();
    PropertyAnimation p2;
    p1.MetaProperty(propName, value);
    p1.Duration(TimeSpan::Milliseconds(100));

    EXPECT_TRUE(p2.Initialize(p1));
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(p1.TotalDuration().Get(), p2.TotalDuration().Get());

    auto prop = property_cast<int>(p2.Metadata().GetProperty(propName));
    ASSERT_NE(prop, nullptr);
    EXPECT_EQ(prop->GetValue(), value);
}

TEST(ObjectTest, Store)
{
    PropertyAnimation p1, p2;
    p1.Duration(TimeSpan::Milliseconds(100));
    p2.Duration(TimeSpan::Milliseconds(200));
    p2.Store(p1);
    EXPECT_EQ(p2.GetIObject(), p1.GetIObject());
    EXPECT_EQ(p2.Duration().Get(), p1.Duration().Get());
    // Should now point to the same object
    p2.Duration(TimeSpan::Milliseconds(300));
    EXPECT_EQ(p2.Duration().Get(), p1.Duration().Get());
    // Store to uninitialized
    PropertyAnimation p3;
    p2.Store(p3);
    EXPECT_EQ(p2.Duration().Get(), p3.Duration().Get());
    // Store uninitialized to uninitialized
    PropertyAnimation p4, p5;
    p4.Store(p5);
    EXPECT_NE(p4.GetIObject(), nullptr);
    EXPECT_EQ(p4, p5);
    p4.Duration(TimeSpan::Milliseconds(500));
    EXPECT_EQ(p4.Duration().Get(), p5.Duration().Get());
}

TEST(ObjectTest, CachedInterfaces)
{
    // Check that also properties accessed through interfaces are updated
    // when the object reference changes, meaning that e.g. any
    // cached interface references get updated
    static constexpr auto propName = "value";
    auto p1 = PropertyAnimation();
    PropertyAnimation p2;
    PropertyAnimation p3;
    p1.MetaProperty(propName, 32);

    p1.Duration(TimeSpan::Milliseconds(100));
    p2.Duration(TimeSpan::Milliseconds(50));
    p3.Duration(TimeSpan::Milliseconds(25));
    PropertyAnimation p4(p3);

    EXPECT_NE(p1.TotalDuration().Get(), p2.TotalDuration().Get());
    EXPECT_NE(p1.TotalDuration().Get(), p3.TotalDuration().Get());

    EXPECT_EQ(p3, p4);
    EXPECT_EQ(p4.TotalDuration().Get(), p3.TotalDuration().Get());
    p3.Duration(TimeSpan::Milliseconds(15));
    EXPECT_EQ(p3.TotalDuration().Get(), TimeSpan::Milliseconds(15));
    EXPECT_EQ(p4.TotalDuration().Get(), p3.TotalDuration().Get());

    // Assign, p1 already has cached IAnimation interface, so this
    // checks that the cached interface is also updated to point
    // to the new target
    p1 = p2;
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(p1.TotalDuration().Get(), p2.TotalDuration().Get());

    // Should fail as p2 already initialized
    EXPECT_FALSE(p2.Initialize(p3));
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(p1.TotalDuration().Get(), p2.TotalDuration().Get());

    p1 = p3;
    // Should not be there anymore as object changed
    EXPECT_EQ(p1.Metadata().GetProperty(propName), nullptr);
}
*/
TEST(ObjectTest, ResetIObject)
{
    META_NS::Object object1;
    META_NS::Object object2;
    auto iobject1 = object1.GetIObject();
    object2 = object1;
    auto iobject2 = object2.GetIObject();
    EXPECT_EQ(iobject1, iobject2);

    object1.ResetIObject();
    EXPECT_FALSE(object1);
    EXPECT_NE(object1.GetIObject(), iobject2);
}
/*
TEST(ObjectTest, ResetIObjectDerived)
{
    using namespace META_NS::TimeSpanLiterals;

    PropertyAnimation p1;
    PropertyAnimation p2;
    p1.Duration(400_ms);
    p2 = p1;
    EXPECT_EQ(p1, p2);

    EXPECT_EQ(GetValue(p1.Duration()), GetValue(p2.Duration()));
    p1.ResetIObject();
    EXPECT_FALSE(p1);
    EXPECT_EQ(GetValue(p2.Duration()), 400_ms);
    p1.Duration(200_ms); // Creates a new object and sets the value
    EXPECT_EQ(GetValue(p1.Duration()), 200_ms);
    // P2 should not have anything to do with p1 anymore
    EXPECT_EQ(GetValue(p2.Duration()), 400_ms);
}

TEST(ObjectTest, CopyiedUserApiObjectContainsPointerToOriginalObject)
{
    // given
    auto origin = PropertyAnimation();

    // when
    auto copy = origin;

    // expected
    const auto originPtr = origin.GetIObject().get();
    const auto copyPtr = copy.GetIObject().get();

    EXPECT_THAT(copy, Eq(origin));
    EXPECT_THAT(copyPtr, Eq(originPtr));
}

TEST(ObjectTest, AssignmentUserApiObjectContainsPointerToOriginalObject)
{
    // given
    auto origin = PropertyAnimation();
    auto assigned = PropertyAnimation();

    // when
    assigned = origin;

    // expected
    const auto originPtr = origin.GetIObject().get();
    const auto assignedPtr = assigned.GetIObject().get();

    EXPECT_THAT(assigned, Eq(origin));
    EXPECT_THAT(assignedPtr, Eq(originPtr));
}

TEST(ObjectTest, MoveConstructedObjectContainsPointerFromOriginAndRemovesContentFromIt)
{
    // given
    auto origin = PropertyAnimation();
    auto originRawPtr = origin.GetIObject().get();

    // when
    auto moved = std::move(origin);

    // expected
    const auto isEmptyAfterMove = !static_cast<bool>(origin);
    const auto originPtr = origin.GetIObject().get();
    const auto movedPtr = moved.GetIObject().get();

    // GetIObject creates new object if it is deinitialized, so it is not nullptr
    EXPECT_THAT(isEmptyAfterMove, Eq(true));
    EXPECT_THAT(originPtr, Ne(originRawPtr));
    EXPECT_THAT(movedPtr, Eq(originRawPtr));
}

TEST(ObjectTest, MoveAssignedObjectContainsPointerFromOriginAndRemovesContentFromIt)
{
    // given
    auto origin = PropertyAnimation();
    auto moveAssigned = PropertyAnimation();

    auto originRawPtr = origin.GetIObject().get();

    // when
    moveAssigned = std::move(origin);

    // expected
    const auto isOriginEmptyAfterMove = !static_cast<bool>(origin);
    const auto isAssignedEmptyAfterMove = !static_cast<bool>(moveAssigned);
    const auto originPtr = origin.GetIObject().get();
    const auto movedPtr = moveAssigned.GetIObject().get();

    // GetIObject creates new object if it is deinitialized, so it is not nullptr
    EXPECT_THAT(isOriginEmptyAfterMove, Eq(true));
    EXPECT_THAT(isAssignedEmptyAfterMove, Eq(false));
    EXPECT_THAT(originPtr, Ne(originRawPtr));
    EXPECT_THAT(movedPtr, Eq(originRawPtr));
}

TEST(ObjectTest, NotCompatibileMoveAssignedObjectIsEmptyAndMovedObjectIsEmptyAlso)
{
    // given
    auto sequentialAnimation = SequentialAnimation();
    auto parallelAnimation = ParallelAnimation();

    auto initialSequentialAnimationPtr = sequentialAnimation.GetIObject().get();
    auto initialParallelAnimationPtr = parallelAnimation.GetIObject().get();

    // when
    parallelAnimation = std::move(sequentialAnimation);

    // expected
    const bool isSequentialAnimationEmptyAfterMove = !static_cast<bool>(sequentialAnimation);
    const bool isParallelAnimationEmptyAfterMove = !static_cast<bool>(parallelAnimation);

    EXPECT_THAT(isSequentialAnimationEmptyAfterMove, Eq(true));
    EXPECT_THAT(isParallelAnimationEmptyAfterMove, Eq(true));
}
*/

TEST(ObjectTest, ObjectName)
{
    {
        Object object;
        auto op = object.GetIObject();
        EXPECT_TRUE(SetName(op, "Test"));
        EXPECT_EQ(GetName(op), "Test");
        EXPECT_TRUE(GetObjectNameAttachment(interface_pointer_cast<IAttach>(op)));
    }
    {
        auto op = GetObjectRegistry().Create<IObjectInstance>(ClassId::BaseObject);
        EXPECT_FALSE(SetName(op, "Test"));
        EXPECT_EQ(GetName(op), op->GetInstanceId().ToString());
    }
    {
        PropertyAnimation anim;
        auto op = anim.GetIObject();
        EXPECT_TRUE(SetName(op, "Test"));
        EXPECT_EQ(GetName(op), "Test");
        EXPECT_FALSE(GetObjectNameAttachment(interface_pointer_cast<IAttach>(op)));
    }
}

TEST(ObjectTest, ObjectNameSerialise)
{
    TestSerialiser ser;
    {
        Object object;
        auto op = object.GetIObject();
        EXPECT_TRUE(SetName(op, "Test"));
        ser.Export(op);
    }
    auto in = ser.Import();
    ASSERT_TRUE(in);
    EXPECT_EQ(GetName(in), "Test");
    EXPECT_TRUE(GetObjectNameAttachment(interface_pointer_cast<IAttach>(in)));
}

} // namespace Test

META_END_NAMESPACE()
