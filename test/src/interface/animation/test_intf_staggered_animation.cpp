/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#include <algorithm>
#include <animation_test_base.h>
#include <bitset>
#include <limits>
#include <numeric>
#include <testing_objects.h>
#include <util.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/api/animation.h>
#include <meta/api/util.h>
#include <meta/interface/intf_containable.h>

#include "serialisation_utils.h"
#include "test_utils.h"

using namespace std::chrono_literals;
using namespace testing;

using META_NS::CreateInstance;

META_BEGIN_NAMESPACE()
namespace Test {

class StaggeredAnimationTest : public AnimationTestBase {
protected:
    void SetUp() override
    {
        AnimationTestBase::SetUp();

        t1_ = META_NS::GetObjectRegistry().Create<ITestType>(ClassId::TestType);
        r1_ = META_NS::GetObjectRegistry().Create<ITestType>(ClassId::TestType);

        ASSERT_NE(t1_, nullptr);
    }

    template<class A, class T>
    void SetProperties(A& target, const IProperty::Ptr& property, T from, T to, TimeSpan duration)
    {
        SetValue(target.Property(), property);
        target.From(from);
        target.To(to);
        SetValue(target.Duration(), duration);
    }

    ITestType::Ptr t1_;
    ITestType::Ptr r1_;
};

TEST_F(StaggeredAnimationTest, CreateParallel)
{
    auto anim = CreateInstance<IParallelAnimation>();

    ASSERT_TRUE(anim);
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IObject.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IAnimation.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IStaggeredAnimation.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IParallelAnimation.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IStartableAnimation.Id().ToUid()));
}

TEST_F(StaggeredAnimationTest, CreateSequential)
{
    auto anim = CreateInstance<ISequentialAnimation>();

    ASSERT_TRUE(anim);
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IObject.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IAnimation.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IStaggeredAnimation.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::ISequentialAnimation.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IStartableAnimation.Id().ToUid()));
}

TEST_F(StaggeredAnimationTest, ParallelSingleChild)
{
    auto staggered = CreateInstance<IParallelAnimation>();
    auto anim1 = CreateInstance<IKeyframeAnimation, BASE_NS::Math::Vec3>();

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to { 100, 50, 0 };

    SetProperties(anim1, t1_->Vec3Property1(), from, to, TimeSpan::Milliseconds(100));

    Container(staggered).Add(Object(anim1));
    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(GetValue(staggered.Running()));
    EXPECT_TRUE(GetValue(anim1.Running()));
    EXPECT_EQ(GetValue(staggered.TotalDuration()), TimeSpan::Milliseconds(100));

    BASE_NS::Math::Vec3 previous = from;
    float previousProgress = 0.f;

    StepAnimations({ staggered }, 10, 10, [&](uint32_t frame) {
        auto progress = GetValue(staggered.Progress());
        // Check that progress of ParallelAnimation moves forward
        EXPECT_GT(progress, previousProgress);
        previousProgress = progress;

        bool running = GetValue(anim1.Running());
        EXPECT_EQ(running, frame < 10) << "Frame: " << frame;

        // Check animations that are still running are proceeding
        auto pos = GetValue(t1_->Vec3Property1());
        EXPECT_GT(pos, previous) << "Frame: " << frame;
        previous = pos;
    });

    EXPECT_FALSE(GetValue(staggered.Running()));
    EXPECT_EQ(GetValue(staggered.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim1.Progress()), 1.f);
    EXPECT_EQ(GetValue(t1_->Vec3Property1()), to);
}

TEST_F(StaggeredAnimationTest, Parallel)
{
    auto staggered = CreateInstance<IParallelAnimation>();
    auto anim1 = CreateInstance<IKeyframeAnimation, BASE_NS::Math::Vec3>();
    auto anim2 = CreateInstance<IKeyframeAnimation, BASE_NS::Math::Vec3>();
    auto anim3 = CreateInstance<IKeyframeAnimation, BASE_NS::Math::Vec3>();

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };

    SetProperties(anim1, t1_->Vec3Property1(), from, to[0], TimeSpan::Milliseconds(100));
    SetProperties(anim2, t1_->Vec3Property2(), from, to[1], TimeSpan::Milliseconds(50));
    SetProperties(anim3, t1_->Vec3Property3(), from, to[2], TimeSpan::Milliseconds(100));

    auto c = Container(staggered);
    c.Add(anim1);
    c.Add(anim2);
    c.Add(anim3);
    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(GetValue(staggered.Running()));
    EXPECT_TRUE(GetValue(anim1.Running()));
    EXPECT_TRUE(GetValue(anim2.Running()));
    EXPECT_TRUE(GetValue(anim3.Running()));
    EXPECT_EQ(GetValue(staggered.TotalDuration()), TimeSpan::Milliseconds(100));

    BASE_NS::Math::Vec3 previous[3] = { from, from, from };
    float previousProgress = 0.f;

    StepAnimations({ staggered }, 10, 10, [&](uint32_t frame) {
        auto progress = GetValue(staggered.Progress());
        // Check that progress of ParallelAnimation moves forward
        EXPECT_GT(progress, previousProgress);
        previousProgress = progress;

        std::bitset<3> currentAnimsRunning;
        currentAnimsRunning[0] = GetValue(anim1.Running());
        currentAnimsRunning[1] = GetValue(anim2.Running());
        currentAnimsRunning[2] = GetValue(anim3.Running());
        // First animation finishes at 50ms, the rest at 100ms
        size_t expectedRunning = frame > 4 ? frame > 9 ? 0 : 2 : 3;
        EXPECT_EQ(currentAnimsRunning.count(), expectedRunning) << "Frame: " << frame;

        // Check animations that are still running are proceeding
        if (currentAnimsRunning[0]) {
            auto pos = GetValue(t1_->Vec3Property1());
            EXPECT_GT(pos, previous[0]) << "Frame: " << frame;
            previous[0] = pos;
        }
        if (currentAnimsRunning[1]) {
            auto rot = GetValue(t1_->Vec3Property2());
            EXPECT_GT(rot, previous[1]) << "Frame: " << frame;
            previous[1] = rot;
        }
        if (currentAnimsRunning[0]) {
            auto sca = GetValue(t1_->Vec3Property3());
            EXPECT_GT(sca, previous[2]) << "Frame: " << frame;
            previous[2] = sca;
        }
    });

    EXPECT_FALSE(GetValue(staggered.Running()));
    EXPECT_EQ(GetValue(staggered.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim1.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim2.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim3.Progress()), 1.f);
    EXPECT_EQ(GetValue(t1_->Vec3Property1()), to[0]);
    EXPECT_EQ(GetValue(t1_->Vec3Property2()), to[1]);
    EXPECT_EQ(GetValue(t1_->Vec3Property3()), to[2]);
}

TEST_F(StaggeredAnimationTest, ParallelStopsItsChildrenWhenNotRunning)
{
    auto staggered = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto childAnim = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    SetProperties(childAnim, t1_->Vec3Property1(), BASE_NS::Math::Vec3 { 0, 0, 0 }, BASE_NS::Math::Vec3 { 100, 50, 0 },
        TimeSpan::Milliseconds(100));

    Container(staggered).Add(childAnim);
    staggered.Start();
    StepAnimations({ staggered }, 10, 10, [](uint32_t /*frame*/) {});
    staggered.Stop();

    EXPECT_FALSE(GetValue(staggered.Running()));
    EXPECT_FALSE(GetValue(childAnim.Running()));
}

TEST_F(StaggeredAnimationTest, ParallelSerialization)
{
    TestSerialiser ser;
    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };
    {
        auto staggered = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
        auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
        auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
        auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

        SetProperties(anim1, t1_->Vec3Property1(), from, to[0], TimeSpan::Milliseconds(100));
        SetProperties(anim2, t1_->Vec3Property2(), from, to[1], TimeSpan::Milliseconds(50));
        SetProperties(anim3, t1_->Vec3Property3(), from, to[2], TimeSpan::Milliseconds(100));

        auto c = Container(staggered);

        c.Add(anim1);
        c.Add(anim2);
        c.Add(anim3);

        Metadata obj(CreateInstance(ClassId::Object));
        obj.AddProperty(t1_->Vec3Property1());
        obj.AddProperty(t1_->Vec3Property2());
        obj.AddProperty(t1_->Vec3Property3());
        AttachmentContainer(obj).Attach(staggered);
        ASSERT_TRUE(ser.Export(Object(obj)));
        ser.Dump("file://./test.json");
    }

    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);

    auto att = interface_cast<IAttach>(obj);
    ASSERT_TRUE(att);
    auto attvec = att->GetAttachments<IAnimation>();
    ASSERT_EQ(attvec.size(), 1);
    ParallelAnimation staggered(attvec.front());
    ASSERT_TRUE(staggered);

    auto anims = staggered.GetAnimations();
    ASSERT_EQ(anims.size(), 3);
    KeyframeAnimation<BASE_NS::Math::Vec3> anim1(anims[0]);
    ASSERT_TRUE(anim1);
    KeyframeAnimation<BASE_NS::Math::Vec3> anim2(anims[1]);
    ASSERT_TRUE(anim2);
    KeyframeAnimation<BASE_NS::Math::Vec3> anim3(anims[2]);
    ASSERT_TRUE(anim3);
    Property<BASE_NS::Math::Vec3> prop1(anim1.Property()->GetValue().lock());
    ASSERT_TRUE(prop1);
    Property<BASE_NS::Math::Vec3> prop2(anim2.Property()->GetValue().lock());
    ASSERT_TRUE(prop2);
    Property<BASE_NS::Math::Vec3> prop3(anim3.Property()->GetValue().lock());
    ASSERT_TRUE(prop3);

    staggered.Start();
    staggered.Step(GetTestClock());

    BASE_NS::Math::Vec3 previous[3] = { from, from, from };
    float previousProgress = 0.f;

    StepAnimations({ staggered }, 10, 10, [&](uint32_t frame) {
        auto progress = GetValue(staggered.Progress());
        // Check that progress of ParallelAnimation moves forward
        EXPECT_GT(progress, previousProgress);
        previousProgress = progress;

        std::bitset<3> currentAnimsRunning;
        currentAnimsRunning[0] = GetValue(anim1.Running());
        currentAnimsRunning[1] = GetValue(anim2.Running());
        currentAnimsRunning[2] = GetValue(anim3.Running());
        // First animation finishes at 50ms, the rest at 100ms
        size_t expectedRunning = frame > 4 ? frame > 9 ? 0 : 2 : 3;
        EXPECT_EQ(currentAnimsRunning.count(), expectedRunning) << "Frame: " << frame;

        // Check animations that are still running are proceeding
        if (currentAnimsRunning[0]) {
            auto pos = prop1->GetValue();
            EXPECT_GT(pos, previous[0]) << "Frame: " << frame;
            previous[0] = pos;
        }
        if (currentAnimsRunning[1]) {
            auto rot = prop2->GetValue();
            EXPECT_GT(rot, previous[1]) << "Frame: " << frame;
            previous[1] = rot;
        }
        if (currentAnimsRunning[0]) {
            auto sca = prop3->GetValue();
            EXPECT_GT(sca, previous[2]) << "Frame: " << frame;
            previous[2] = sca;
        }
    });

    EXPECT_FALSE(GetValue(staggered.Running()));
    EXPECT_EQ(GetValue(staggered.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim1.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim2.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim3.Progress()), 1.f);
    EXPECT_EQ(GetValue(prop1), to[0]);
    EXPECT_EQ(GetValue(prop2), to[1]);
    EXPECT_EQ(GetValue(prop3), to[2]);
}

TEST_F(StaggeredAnimationTest, ParallelLoop)
{
    auto staggered = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    const int loopCount = 3;
    auto loop = META_NS::AnimationModifiers::Loop(CreateInstance(ClassId::LoopAnimationModifier)).LoopCount(loopCount);

    AttachmentContainer(staggered).Attach(loop);

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };

    SetProperties(anim1, t1_->Vec3Property1(), from, to[0], TimeSpan::Milliseconds(100));
    SetProperties(anim2, t1_->Vec3Property2(), from, to[1], TimeSpan::Milliseconds(50));
    SetProperties(anim3, t1_->Vec3Property3(), from, to[2], TimeSpan::Milliseconds(100));

    Container c(staggered);
    c.Add(anim1);
    c.Add(anim2);
    c.Add(anim3);
    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(GetValue(staggered.Running()));
    EXPECT_TRUE(GetValue(anim1.Running()));
    EXPECT_TRUE(GetValue(anim2.Running()));
    EXPECT_TRUE(GetValue(anim3.Running()));
    EXPECT_EQ(GetValue(staggered.TotalDuration()), TimeSpan::Milliseconds(100) * loopCount);

    for (int i = 0; i < loopCount; ++i) {
        float previousProgress = -1.f;
        auto initFrom = from - BASE_NS::Math::Vec3(1, 1, 1);
        BASE_NS::Math::Vec3 previous[3] = { initFrom, initFrom, initFrom };
        auto loop = i + 1;

        StepAnimations({ staggered }, 10, 10, [&](uint32_t frame) {
            auto progress = GetValue(staggered.Progress());
            // Check that progress of ParallelAnimation moves forward
            EXPECT_GT(progress, previousProgress) << "Frame " << frame << ", Loop: " << loop;
            previousProgress = progress;

            std::bitset<3> currentAnimsRunning;
            currentAnimsRunning[0] = GetValue(anim1.Running());
            currentAnimsRunning[1] = GetValue(anim2.Running());
            currentAnimsRunning[2] = GetValue(anim3.Running());

            size_t expectedRunning = frame > 4 ? frame > 9 ? 0 : 2 : 3;
            ASSERT_EQ(currentAnimsRunning.count(), expectedRunning) << "Frame: " << frame << ", Loop: " << loop;

            // Check animations that are still running are proceeding
            if (currentAnimsRunning[0]) {
                auto pos = GetValue(t1_->Vec3Property1());
                EXPECT_GT(pos, previous[0]);
                previous[0] = pos;
            }
            if (currentAnimsRunning[1]) {
                auto rot = GetValue(t1_->Vec3Property2());
                EXPECT_GT(rot, previous[1]);
                previous[1] = rot;
            }
            if (currentAnimsRunning[0]) {
                auto sca = GetValue(t1_->Vec3Property3());
                EXPECT_GT(sca, previous[2]);
                previous[2] = sca;
            }
        });

        if (loop == loopCount) {
            // Last loop, animation should have finished
            EXPECT_FALSE(GetValue(staggered.Running()));
        } else {
            EXPECT_TRUE(GetValue(staggered.Running()));
            staggered.Step(GetTestClock()); // One extra step to let the animations jump back to beginning
        }
    }

    EXPECT_FLOAT_EQ(1.0f, GetValue(staggered.Progress()));

    EXPECT_FALSE(GetValue(staggered.Running()));
    EXPECT_EQ(GetValue(staggered.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim1.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim2.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim3.Progress()), 1.f);
    EXPECT_EQ(GetValue(t1_->Vec3Property1()), to[0]);
    EXPECT_EQ(GetValue(t1_->Vec3Property2()), to[1]);
    EXPECT_EQ(GetValue(t1_->Vec3Property3()), to[2]);
}

TEST_F(StaggeredAnimationTest, ParallelReverse)
{
    auto staggered = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto reverse = META_NS::AnimationModifiers::Reverse(CreateInstance(ClassId::ReverseAnimationModifier));

    AttachmentContainer(staggered).Attach(reverse);

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };

    SetProperties(anim1, t1_->Vec3Property1(), from, to[0], TimeSpan::Milliseconds(100));
    SetProperties(anim2, t1_->Vec3Property2(), from, to[1], TimeSpan::Milliseconds(30));
    SetProperties(anim3, t1_->Vec3Property3(), from, to[2], TimeSpan::Milliseconds(100));

    Container c(staggered);
    c.Add(anim1);
    c.Add(anim2);
    c.Add(anim3);
    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(GetValue(staggered.Running()));
    EXPECT_TRUE(GetValue(anim1.Running()));
    EXPECT_FALSE(GetValue(anim2.Running()));
    EXPECT_TRUE(GetValue(anim3.Running()));
    EXPECT_EQ(GetValue(staggered.TotalDuration()), TimeSpan::Milliseconds(100));

    EXPECT_EQ(GetValue(t1_->Vec3Property1()), to[0]);
    EXPECT_EQ(GetValue(t1_->Vec3Property2()), from);
    EXPECT_EQ(GetValue(t1_->Vec3Property3()), to[2]);

    float previousProgress = -1.f;
    auto initFrom = to[2] + BASE_NS::Math::Vec3(1, 1, 1);
    BASE_NS::Math::Vec3 previous[3] = { initFrom, initFrom, initFrom };

    StepAnimations({ staggered }, 10, 10, [&](uint32_t frame) {
        auto progress = GetValue(staggered.Progress());
        // Check that progress of ParallelAnimation moves forward
        EXPECT_GT(progress, previousProgress) << "Frame " << frame;
        previousProgress = progress;

        std::bitset<3> currentAnimsRunning;
        currentAnimsRunning[0] = GetValue(anim1.Running());
        currentAnimsRunning[1] = GetValue(anim2.Running());
        currentAnimsRunning[2] = GetValue(anim3.Running());

        size_t expectedRunning = frame > 6 ? frame > 9 ? 0 : 3 : 2;
        ASSERT_EQ(currentAnimsRunning.count(), expectedRunning) << "Frame: " << frame;

        // Check animations that are still running are proceeding
        if (currentAnimsRunning[0]) {
            auto pos = GetValue(t1_->Vec3Property1());
            EXPECT_LT(pos, previous[0]);
            previous[0] = pos;
        }
        if (currentAnimsRunning[1]) {
            auto rot = GetValue(t1_->Vec3Property2());
            EXPECT_LT(rot, previous[1]);
            previous[1] = rot;
        }
        if (currentAnimsRunning[0]) {
            auto sca = GetValue(t1_->Vec3Property3());
            EXPECT_LT(sca, previous[2]);
            previous[2] = sca;
        }
    });

    EXPECT_FLOAT_EQ(1.0f, GetValue(staggered.Progress()));

    EXPECT_FALSE(GetValue(staggered.Running()));
    EXPECT_EQ(GetValue(staggered.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim1.Progress()), 0.f);
    EXPECT_EQ(GetValue(anim2.Progress()), 0.f);
    EXPECT_EQ(GetValue(anim3.Progress()), 0.f);
    EXPECT_EQ(GetValue(t1_->Vec3Property1()), from);
    EXPECT_EQ(GetValue(t1_->Vec3Property2()), from);
    EXPECT_EQ(GetValue(t1_->Vec3Property3()), from);
}

TEST_F(StaggeredAnimationTest, LoopPastEnd)
{
    META_NS::KeyframeAnimation<float> a1(CreateInstance(ClassId::KeyframeAnimation));
    META_NS::KeyframeAnimation<float> a2(CreateInstance(ClassId::KeyframeAnimation));
    auto staggered = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
    Container(staggered).Add(a1.From(0).To(10).Property(t1_->FloatProperty1()).Duration(TimeSpan::Milliseconds(100)));
    Container(staggered).Add(
        a2.From(10)
            .To(0)
            .Property(t1_->FloatProperty1())
            .Duration(TimeSpan::Milliseconds(100))
            .Curve(interface_pointer_cast<ICurve1D>(CreateInstance(ClassId::InOutCubicEasingCurve))));

    AttachmentContainer(staggered).Attach(
        META_NS::AnimationModifiers::Loop(CreateInstance(ClassId::LoopAnimationModifier)).LoopIndefinitely());

    staggered.Start();
    ASSERT_TRUE(GetValue(staggered.Running()));

    float expectedProgress = 0.f;
    float expectedProgressStep = 20.f / 200.f; // 200ms animation with 20ms step

    // Run the animation once so that our steps hit 1.0
    for (auto loop = 1; loop <= 3; loop++) {
        staggered.Step(GetTestClock());
        StepAnimations({ staggered }, 10, 20, [&](uint32_t frame) {
            auto progress = GetValue(staggered.Progress());
            expectedProgress += expectedProgressStep;
            expectedProgress = expectedProgress - BASE_NS::Math::floor(expectedProgress);

            EXPECT_EQ(GetValue(a1.Running()), frame < 5) << "Frame: " << frame << ", loop: " << loop;
            EXPECT_EQ(GetValue(a2.Running()), frame >= 5 && frame < 10) << "Frame: " << frame << ", loop: " << loop;
        });
    }

    staggered.Restart();
    staggered.Step(GetTestClock());
    expectedProgress = 0.f;
    expectedProgressStep = 30.f / 200.f; // 200ms animation with 30ms step

    // Run again but now so that we step past 1.0
    for (auto loop = 1; loop <= 3; loop++) {
        auto frameCount = loop == 1 ? 6 : 7;
        StepAnimations({ staggered }, frameCount, 30, [&](uint32_t frame) {
            auto progress = GetValue(staggered.Progress());
            expectedProgress += expectedProgressStep;
            expectedProgress = expectedProgress - BASE_NS::Math::floor(expectedProgress);

            EXPECT_NEAR(progress, expectedProgress, 0.05);

            // On the seventh frame we should jump past one loop, and be at 0.05 progress or thereabouts
            bool a1running = frame < 4;
            bool a2running = !a1running && progress < 1.f;

            EXPECT_EQ(GetValue(a1.Running()), a1running) << "Frame: " << frame << ", loop: " << loop;
            EXPECT_EQ(GetValue(a2.Running()), a2running) << "Frame: " << frame << ", loop: " << loop;
        });
    }
}

TEST_F(StaggeredAnimationTest, Sequential)
{
    auto staggered = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
    auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };

    // In total 150ms of animations
    SetProperties(anim1, t1_->Vec3Property1(), from, to[0], TimeSpan::Milliseconds(50));
    SetProperties(anim2, t1_->Vec3Property2(), from, to[1], TimeSpan::Milliseconds(100));
    SetProperties(anim3, t1_->Vec3Property3(), from, to[2], TimeSpan::Milliseconds(50));

    Container c(staggered);
    c.Add(anim1);
    c.Add(anim2);
    c.Add(anim3);

    EXPECT_EQ(GetValue(staggered.TotalDuration()), TimeSpan::Milliseconds(200));

    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(GetValue(anim1.Running()));
    EXPECT_TRUE(GetValue(staggered.Running()));

    std::bitset<3> animsRun;
    float previousProgress = 0.f;

    // 20 frames at 10ms interval should finish the animation
    RunFrames(20, [&animsRun, &anim1, &anim2, &anim3, &staggered, &previousProgress](uint32_t frame) {
        auto progress = GetValue(staggered.Progress());

        // Each frame should progress the animation
        EXPECT_GT(progress, previousProgress) << "at frame " << frame;
        previousProgress = progress;

        std::bitset<3> currentAnimsRunning;
        currentAnimsRunning[0] = GetValue(anim1.Running());
        currentAnimsRunning[1] = GetValue(anim2.Running());
        currentAnimsRunning[2] = GetValue(anim3.Running());

        // Only one animation should be running at a time
        EXPECT_EQ(currentAnimsRunning.count(), frame < 20 ? 1 : 0) << "at frame " << frame;

        animsRun |= currentAnimsRunning;

        // Check that total animation progress is correct for the
        // currently running animation from the sequence
        if (currentAnimsRunning[0]) {
            EXPECT_GE(progress, 0.f);
            EXPECT_LE(progress, 1.f / 4.f);
        }
        if (currentAnimsRunning[1]) {
            EXPECT_GE(progress, 1.f / 4.f);
            EXPECT_LE(progress, 3.f / 4.f);
        }
        if (currentAnimsRunning[2]) {
            EXPECT_GE(progress, 3.f / 4.f);
            EXPECT_LE(progress, 1.f);
        }
    });

    EXPECT_EQ(animsRun.count(), 3);
    EXPECT_FALSE(GetValue(staggered.Running()));
    EXPECT_EQ(GetValue(staggered.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim1.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim2.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim3.Progress()), 1.f);
    EXPECT_EQ(GetValue(t1_->Vec3Property1()), to[0]);
    EXPECT_EQ(GetValue(t1_->Vec3Property2()), to[1]);
    EXPECT_EQ(GetValue(t1_->Vec3Property3()), to[2]);
}

TEST_F(StaggeredAnimationTest, SequentialSerialization)
{
    TestSerialiser ser;
    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };
    {
        auto staggered = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
        auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
        auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
        auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

        // In total 150ms of animations
        SetProperties(anim1, t1_->Vec3Property1(), from, to[0], TimeSpan::Milliseconds(50));
        SetProperties(anim2, t1_->Vec3Property2(), from, to[1], TimeSpan::Milliseconds(100));
        SetProperties(anim3, t1_->Vec3Property3(), from, to[2], TimeSpan::Milliseconds(50));

        Container c(staggered);
        c.Add(anim1);
        c.Add(anim2);
        c.Add(anim3);

        Metadata obj(CreateInstance(ClassId::Object));
        obj.AddProperty(t1_->Vec3Property1());
        obj.AddProperty(t1_->Vec3Property2());
        obj.AddProperty(t1_->Vec3Property3());
        AttachmentContainer(obj).Attach(staggered);
        ASSERT_TRUE(ser.Export(Object(obj)));
        ser.Dump("file://./test.json");
    }

    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);

    auto att = interface_cast<IAttach>(obj);
    ASSERT_TRUE(att);
    auto attvec = att->GetAttachments<IAnimation>();
    ASSERT_EQ(attvec.size(), 1);
    SequentialAnimation staggered(attvec.front());
    ASSERT_TRUE(staggered);

    auto anims = staggered.GetAnimations();
    ASSERT_EQ(anims.size(), 3);
    KeyframeAnimation<BASE_NS::Math::Vec3> anim1(anims[0]);
    ASSERT_TRUE(anim1);
    KeyframeAnimation<BASE_NS::Math::Vec3> anim2(anims[1]);
    ASSERT_TRUE(anim2);
    KeyframeAnimation<BASE_NS::Math::Vec3> anim3(anims[2]);
    ASSERT_TRUE(anim3);
    Property<BASE_NS::Math::Vec3> prop1(anim1.Property()->GetValue().lock());
    ASSERT_TRUE(prop1);
    Property<BASE_NS::Math::Vec3> prop2(anim2.Property()->GetValue().lock());
    ASSERT_TRUE(prop2);
    Property<BASE_NS::Math::Vec3> prop3(anim3.Property()->GetValue().lock());
    ASSERT_TRUE(prop3);

    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(GetValue(anim1.Running()));
    EXPECT_TRUE(GetValue(staggered.Running()));

    std::bitset<3> animsRun;
    float previousProgress = 0.f;

    // 20 frames at 10ms interval should finish the animation
    RunFrames(20, [&animsRun, &anim1, &anim2, &anim3, &staggered, &previousProgress](uint32_t frame) {
        auto progress = GetValue(staggered.Progress());

        // Each frame should progress the animation
        EXPECT_GT(progress, previousProgress) << "at frame " << frame;
        previousProgress = progress;

        std::bitset<3> currentAnimsRunning;
        currentAnimsRunning[0] = GetValue(anim1.Running());
        currentAnimsRunning[1] = GetValue(anim2.Running());
        currentAnimsRunning[2] = GetValue(anim3.Running());

        // Only one animation should be running at a time
        EXPECT_EQ(currentAnimsRunning.count(), frame < 20 ? 1 : 0) << "at frame " << frame;

        animsRun |= currentAnimsRunning;

        // Check that total animation progress is correct for the
        // currently running animation from the sequence
        if (currentAnimsRunning[0]) {
            EXPECT_GE(progress, 0.f);
            EXPECT_LE(progress, 1.f / 4.f);
        }
        if (currentAnimsRunning[1]) {
            EXPECT_GE(progress, 1.f / 4.f);
            EXPECT_LE(progress, 3.f / 4.f);
        }
        if (currentAnimsRunning[2]) {
            EXPECT_GE(progress, 3.f / 4.f);
            EXPECT_LE(progress, 1.f);
        }
    });

    EXPECT_EQ(animsRun.count(), 3);
    EXPECT_FALSE(GetValue(staggered.Running()));
    EXPECT_EQ(GetValue(staggered.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim1.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim2.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim3.Progress()), 1.f);
    EXPECT_EQ(GetValue(prop1), to[0]);
    EXPECT_EQ(GetValue(prop2), to[1]);
    EXPECT_EQ(GetValue(prop3), to[2]);
}

TEST_F(StaggeredAnimationTest, SequentialWithSameAnimation)
{
    auto staggered = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
    auto anim = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to = { 100, 50, 0 };

    SetProperties(anim, t1_->Vec3Property1(), from, to, TimeSpan::Milliseconds(50));

    Container c(staggered);
    c.Add(anim);
    c.Add(anim);
    c.Add(anim);

    EXPECT_EQ(GetValue(staggered.TotalDuration()), TimeSpan::Milliseconds(150));

    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(GetValue(anim.Running()));
    EXPECT_TRUE(GetValue(staggered.Running()));

    float previousProgress = 0.f;

    // 20 frames + 10ms sleep at end of each frame -> Should be enough time
    // for all animations to run
    StepAnimations({ staggered }, 15, 10, [&](uint32_t frame) {
        EXPECT_EQ(GetValue(staggered.Running()), frame < 15) << "Frame: " << frame;
        auto progress = GetValue(staggered.Progress());

        // Each frame should progress the animation
        EXPECT_GT(progress, previousProgress) << "Frame: " << frame;
        previousProgress = progress;
    });

    EXPECT_FALSE(GetValue(staggered.Running()));
    EXPECT_EQ(GetValue(staggered.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim.Progress()), 1.f);
}

TEST_F(StaggeredAnimationTest, SequentialModified)
{
    auto staggered = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
    auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    const int loopCount = 3;
    auto loop = META_NS::AnimationModifiers::Loop(CreateInstance(ClassId::LoopAnimationModifier)).LoopCount(loopCount);
    AttachmentContainer(staggered).Attach(loop);

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };

    // In total 150ms of animations
    SetProperties(anim1, t1_->Vec3Property1(), from, to[0], TimeSpan::Milliseconds(50));
    SetProperties(anim2, t1_->Vec3Property2(), from, to[1], TimeSpan::Milliseconds(50));
    SetProperties(anim3, t1_->Vec3Property3(), from, to[2], TimeSpan::Milliseconds(50));

    Container c(staggered);
    c.Add(anim1);
    c.Add(anim2);
    c.Add(anim3);
    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(GetValue(anim1.Running()));
    EXPECT_TRUE(GetValue(staggered.Running()));

    std::bitset<3> animsRun;

    for (int i = 0; i < loopCount; ++i) {
        float previousProgress = -1;
        auto loop = i + 1;

        StepAnimations({ staggered }, 15, 10, [&](uint32_t frame) {
            auto progress = GetValue(staggered.Progress());

            // Each frame should progress the animation
            EXPECT_GT(progress, previousProgress) << "Frame: " << frame << ", Loop: " << loop;
            previousProgress = progress;

            std::bitset<3> currentAnimsRunning;
            currentAnimsRunning[0] = GetValue(anim1.Running());
            currentAnimsRunning[1] = GetValue(anim2.Running());
            currentAnimsRunning[2] = GetValue(anim3.Running());

            // Only one animation should be running at a time
            EXPECT_EQ(currentAnimsRunning.count(), frame < 15 ? 1 : 0) << "Frame: " << frame << ", Loop: " << loop;

            animsRun |= currentAnimsRunning;

            // Check that total animation progress is correct for the
            // currently running animation from the sequence
            if (currentAnimsRunning[0]) {
                EXPECT_GE(progress, 0.f);
                EXPECT_LE(progress, 1.f / 3.f + std::numeric_limits<float>::epsilon());
            }
            if (currentAnimsRunning[1]) {
                EXPECT_GE(progress, 1.f / 3.f - std::numeric_limits<float>::epsilon());
                EXPECT_LE(progress, 2.f / 3.f + std::numeric_limits<float>::epsilon());
            }
            if (currentAnimsRunning[2]) {
                EXPECT_GE(progress, 2.f / 3.f - std::numeric_limits<float>::epsilon());
                EXPECT_LE(progress, 1.f);
            }
        });
        if (loop == loopCount) {
            // Last loop, animation should have finished
            EXPECT_FALSE(GetValue(staggered.Running()));
        } else {
            EXPECT_TRUE(GetValue(staggered.Running()));
            staggered.Step(GetTestClock()); // One extra step to let the animations jump back to beginning
        }
    }

    EXPECT_EQ(animsRun.count(), 3);
    EXPECT_FALSE(GetValue(staggered.Running()));
    EXPECT_EQ(GetValue(staggered.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim1.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim2.Progress()), 1.f);
    EXPECT_EQ(GetValue(anim3.Progress()), 1.f);
    EXPECT_EQ(GetValue(t1_->Vec3Property1()), to[0]);
    EXPECT_EQ(GetValue(t1_->Vec3Property2()), to[1]);
    EXPECT_EQ(GetValue(t1_->Vec3Property3()), to[2]);
}

TEST_F(StaggeredAnimationTest, DurationParallel)
{
    auto staggered = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };
    TimeSpan baseDuration = TimeSpan::Milliseconds(50);

    // Last animation has the longest duration
    std::vector<TimeSpan> duration = { baseDuration, baseDuration, baseDuration * 12 };

    SetProperties(anim1, t1_->Vec3Property1(), from, to[0], duration[0]);
    SetProperties(anim2, t1_->Vec3Property2(), from, to[1], duration[1]);
    SetProperties(anim3, t1_->Vec3Property3(), from, to[2], duration[2]);

    Container c(staggered);
    c.Add(anim1);
    c.Add(anim2);
    c.Add(anim3);

    auto staggeredDuration = *std::max_element(duration.begin(), duration.end());
    EXPECT_EQ(GetValue(staggered.TotalDuration()), staggeredDuration);
}

TEST_F(StaggeredAnimationTest, ParallelWithSameAnimation)
{
    auto staggered = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto anim = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to = { 100, 50, 0 };

    SetProperties(anim, t1_->Vec3Property1(), from, to, TimeSpan::Milliseconds(50));

    Container c(staggered);
    c.Add(anim);
    c.Add(anim);
    c.Add(anim);

    EXPECT_EQ(staggered.GetAnimations().size(), 1);
    EXPECT_EQ(GetValue(staggered.TotalDuration()), TimeSpan::Milliseconds(50));
}

TEST_F(StaggeredAnimationTest, DurationParallelModified)
{
    auto staggered = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    const int loopCount = 3;
    auto loop = META_NS::AnimationModifiers::Loop(CreateInstance(ClassId::LoopAnimationModifier)).LoopCount(loopCount);
    AttachmentContainer(staggered).Attach(loop);

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };
    TimeSpan baseDuration = TimeSpan::Milliseconds(50);

    // Last animation has the longest duration
    std::vector<TimeSpan> duration = { baseDuration, baseDuration, baseDuration * 12 };

    SetProperties(anim1, t1_->Vec3Property1(), from, to[0], duration[0]);
    SetProperties(anim2, t1_->Vec3Property2(), from, to[1], duration[1]);
    SetProperties(anim3, t1_->Vec3Property3(), from, to[2], duration[2]);

    Container c(staggered);
    c.Add(anim1);
    c.Add(anim2);
    c.Add(anim3);

    auto staggeredDuration = *std::max_element(duration.begin(), duration.end());
    EXPECT_EQ(GetValue(staggered.TotalDuration()), staggeredDuration * loopCount);
}

TEST_F(StaggeredAnimationTest, DurationSequential)
{
    auto staggered = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
    auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };
    TimeSpan baseDuration = TimeSpan::Milliseconds(50);

    // Last animation has the longest duration
    std::vector<TimeSpan> duration = { baseDuration, baseDuration, baseDuration * 12 };

    SetProperties(anim1, t1_->Vec3Property1(), from, to[0], duration[0]);
    SetProperties(anim2, t1_->Vec3Property2(), from, to[1], duration[1]);
    SetProperties(anim3, t1_->Vec3Property3(), from, to[2], duration[2]);

    Container c(staggered);
    c.Add(anim1);
    c.Add(anim2);
    c.Add(anim3);

    auto staggeredDuration = std::accumulate(duration.begin(), duration.end(), TimeSpan::Zero());
    EXPECT_EQ(GetValue(staggered.TotalDuration()), staggeredDuration);
}

TEST_F(StaggeredAnimationTest, Hierarchy)
{
    /*
    Create the following animation hierarchy:
    par1                    ParallelAnimation
      |
      +-- seq1              SequentialAnimation
      |     |
      |     +-- par2        ParallelAnimation
      |     |     |
      |     |     +-- a1    KeyframeAnimation
      |     |     +-- a2    KeyframeAnimation
      |     +-- a3          KeyframeAnimation
      +-- a4                KeyframeAnimation
    */

    auto p1 = ConstructProperty<float>("p1", 0);
    auto p2 = ConstructProperty<float>("p2", 0);
    auto p3 = ConstructProperty<float>("p3", 0);
    auto p4 = ConstructProperty<float>("p4", 0);

    auto seq1 = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
    auto par1 = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto par2 = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));

    auto a1 = META_NS::KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation));
    auto a2 = META_NS::KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation));
    auto a3 = META_NS::KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation));
    auto a4 = META_NS::KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation));

    float from[4] = { 0, 5, 10, 20 };
    float to[4] = { 10, 10, 25, 40 };

    SetProperties(a1, p1, from[0], to[0], TimeSpan::Milliseconds(30));
    SetProperties(a2, p2, from[1], to[1], TimeSpan::Milliseconds(30));
    SetProperties(a3, p3, from[2], to[2], TimeSpan::Milliseconds(75));
    SetProperties(a4, p4, from[3], to[3], TimeSpan::Milliseconds(150));

    par2.Add(a1).Add(a2);
    seq1.Add(a3);
    par1.Add(seq1).Add(a4);
    seq1.Add(par2);

    par1.Start();
    par1.Step(GetTestClock());

    float previousProgress = 0.f;

    StepAnimations({ par1 }, 15, 10, [&](uint32_t frame) {
        EXPECT_EQ(GetValue(par1.Running()), frame < 15) << "Frame " << frame;

        auto progress = GetValue(par1.Progress());
        EXPECT_GT(progress, previousProgress) << "Frame " << frame;
        previousProgress = progress;
    });

    EXPECT_FALSE(GetValue(par1.Running()));

    EXPECT_EQ(GetValue(seq1.Progress()), 1.f);
    EXPECT_EQ(GetValue(par1.Progress()), 1.f);
    EXPECT_EQ(GetValue(par2.Progress()), 1.f);
    EXPECT_EQ(GetValue(a1.Progress()), 1.f);
    EXPECT_EQ(GetValue(a2.Progress()), 1.f);
    EXPECT_EQ(GetValue(a3.Progress()), 1.f);
    EXPECT_EQ(GetValue(a4.Progress()), 1.f);
    EXPECT_EQ(GetValue(p1), to[0]);
    EXPECT_EQ(GetValue(p2), to[1]);
    EXPECT_EQ(GetValue(p3), to[2]);
    EXPECT_EQ(GetValue(p4), to[3]);
}

TEST_F(StaggeredAnimationTest, Reparent)
{
    auto p1 = ConstructProperty<float>("p1", 0);
    auto p2 = ConstructProperty<float>("p2", 0);
    auto seq = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
    auto par = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));

    auto a1 = META_NS::KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation));
    auto a2 = META_NS::KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation));

    auto a1cont = interface_cast<IContainable>(a1);
    auto a2cont = interface_cast<IContainable>(a2);
    ASSERT_NE(a1cont, nullptr);
    ASSERT_NE(a2cont, nullptr);

    float from[2] = { 0, 5 };
    float to[2] = { 10, 50 };
    TimeSpan duration[2] = { TimeSpan::Milliseconds(100), TimeSpan::Milliseconds(200) };

    SetProperties(a1, p1, from[0], to[0], duration[0]);
    SetProperties(a2, p2, from[1], to[1], duration[1]);

    seq.Add(a1).Add(a2);
    EXPECT_EQ(a1cont->GetParent(), seq);
    EXPECT_EQ(a2cont->GetParent(), seq);

    EXPECT_EQ(GetValue(seq.TotalDuration()), duration[0] + duration[1]);

    seq.Start();
    seq.Step(GetTestClock());

    EXPECT_TRUE(GetValue(seq.Running()));
    EXPECT_TRUE(GetValue(a1.Running()));

    // Reparent the animation
    par.Add(a1);
    EXPECT_EQ(a1cont->GetParent(), par);
    EXPECT_EQ(a2cont->GetParent(), seq);

    EXPECT_TRUE(GetValue(seq.Running()));
    EXPECT_TRUE(GetValue(a1.Running()));
    seq.Stop();

    // Animation durations should have been updated due to hierarchy change
    EXPECT_EQ(GetValue(seq.TotalDuration()), duration[1]);
    EXPECT_EQ(GetValue(par.TotalDuration()), duration[0]);

    par.Start();
    EXPECT_FALSE(GetValue(seq.Running()));
    EXPECT_TRUE(GetValue(par.Running()));
    EXPECT_TRUE(GetValue(a1.Running()));
}

TEST_F(StaggeredAnimationTest, Name)
{
    ParallelAnimation a1(CreateInstance(ClassId::ParallelAnimation));
    SequentialAnimation a2(CreateInstance(ClassId::SequentialAnimation));

    Named n1(a1);
    Named n2(a2);
    EXPECT_NE(GetValue(n1.Name()), "");
    EXPECT_NE(GetValue(n2.Name()), "");

    SetValue(n1.Name(), "Anim1");
    SetValue(n2.Name(), "Anim2");

    EXPECT_EQ(GetValue(n1.Name()), "Anim1");
    EXPECT_EQ(a1.GetPtr()->GetName(), GetValue(n1.Name()));
    EXPECT_EQ(GetValue(n2.Name()), "Anim2");
    EXPECT_EQ(a2.GetPtr()->GetName(), GetValue(n2.Name()));
}

TEST_F(StaggeredAnimationTest, ParallelWithTrackAnimationSeeking)
{
    static BASE_NS::vector<float> trackTimestamps = { 0.0f, 1.f };
    static BASE_NS::vector<int> trackKeyframes = { 0, 10 };

    auto property = ConstructProperty<int>("Prop");
    auto parallel = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto anim = META_NS::TrackAnimation<int>(CreateInstance(ClassId::TrackAnimation));
    anim.Keyframes(trackKeyframes)
        .Timestamps(trackTimestamps)
        .Property(property)
        .Duration(TimeSpan::Milliseconds(1000));

    ASSERT_TRUE(parallel);
    ASSERT_TRUE(anim);
    parallel.Add(anim);
    parallel.Start();
    parallel.Step(GetTestClock());

    EXPECT_TRUE(GetValue(parallel.Running()));
    EXPECT_TRUE(GetValue(anim.Running()));

    parallel.Pause();

    int count = 0;
    property->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] {
        ++count;
        EXPECT_EQ(property->GetValue(), count);
    }));

    // should do nothing
    StepAnimations({ parallel }, 10, [](uint32_t frame) {});

    parallel.Seek(0.1f);

    // should do nothing
    StepAnimations({ parallel }, 10, [](uint32_t frame) {});

    EXPECT_EQ(property->GetValue(), 1);
    EXPECT_EQ(count, 1);
    parallel.Seek(0.2f);
    EXPECT_EQ(property->GetValue(), 2);
    EXPECT_EQ(count, 2);
    parallel.Seek(0.3f);
    EXPECT_EQ(property->GetValue(), 3);
    EXPECT_EQ(count, 3);
}

} // namespace Test

META_END_NAMESPACE()
