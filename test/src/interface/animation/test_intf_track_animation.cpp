/**
 *  Copyright (C) 2023 Huawei Technologies Co, Ltd.
 */
#include <animation_test_base.h>
#include <tester.h>
#include <testing_objects.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/api/animation/animation.h>
#include <meta/api/animation/modifiers/loop.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/property/property.h>

#include "serialisation_utils.h"

using namespace testing;

META_BEGIN_NAMESPACE()

namespace Test {

class ITestKeyframeHandler : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITestKeyframeHandler, "f1b45ddb-88ce-47b2-8317-21e545b9b4cd")
public:
    virtual int Called() const = 0;
    virtual void MyFunc() = 0;
};

META_REGISTER_CLASS(KeyframeHandler, "9d6ec02e-66f1-4d57-bf06-6e1494ccc06a", META_NS::ObjectCategoryBits::APPLICATION)

class KeyframeHandler final : public IntroduceInterfaces<META_NS::ObjectFwd, ITestKeyframeHandler> {
    META_OBJECT(KeyframeHandler, ClassId::KeyframeHandler, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_FUNCTION_DATA(ITestKeyframeHandler, MyFunc)
    META_END_STATIC_DATA()

    void MyFunc() override
    {
        ++count_;
    }

    int Called() const override
    {
        return count_;
    }

    int count_ {};
};

class TrackAnimationTest : public AnimationTestBase {
protected:
    void TestBindingAnimation(bool animateSource, bool useInBetweenProperty);
};

TEST_F(TrackAnimationTest, TrackAnimation)
{
    RegisterObjectType<KeyframeHandler>();
    static BASE_NS::vector<float> timestamps = { 0.1f, 0.3f, 0.8f, 1.f };
    static BASE_NS::vector<BASE_NS::Math::Vec3> keyframes = { { 50, 30, 0 }, { 100, 80, 0 }, { 170, 150, 0 },
        { 500, 500, 0 } };
    static BASE_NS::Math::Vec3 initialValue = { 10, 10, 0 };

    BASE_NS::vector<ITestKeyframeHandler::Ptr> handlerObjects;
    BASE_NS::vector<IFunction::Ptr> handlers;
    for (int i = 0; i != timestamps.size(); ++i) {
        handlerObjects.push_back(META_NS::GetObjectRegistry().Create<ITestKeyframeHandler>(ClassId::KeyframeHandler));
        handlers.push_back(interface_cast<IMetadata>(handlerObjects.back())->GetFunction("MyFunc"));
    }

    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", initialValue);
    auto animation = META_NS::TrackAnimation<BASE_NS::Math::Vec3>()
                         .Keyframes(keyframes)
                         .Timestamps(timestamps)
                         .Property(property)
                         .Duration(TimeSpan::Milliseconds(1000))
                         .KeyframeHandlers(handlers);

    ASSERT_TRUE(GetValue(animation.Valid()));

    ASSERT_EQ(TimeSpan::Seconds(1.0f), GetValue(animation.TotalDuration()));

    animation.Start();
    // First frame
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(GetValue(animation.Running()));
    BASE_NS::Math::Vec3 previous = initialValue;

    uint32_t steps = 0;

    // Each step steps by 10ms, so 100 steps should cover the whole 1000ms animation
    this->StepAnimations({ animation }, 100, [&](uint32_t frame) {
        const auto progress = GetValue(animation.Progress());
        const auto value = GetValue(property);
        if (progress < timestamps[0]) {
            // First timestamp at 0.1, before that just get the value
            // EXPECT_EQ(value, initialValue) << "Frame: " << frame;
            // First timestamp at 0.1, before that just get the first keyframe
            EXPECT_EQ(value, keyframes.front()) << "Frame: " << frame;
            EXPECT_EQ(!handlerObjects[0]->Called(), 1) << "Frame: " << frame;
            EXPECT_EQ(GetValue(animation.CurrentKeyframeIndex()), -1) << "Frame: " << frame;
        } else {
            steps++;
            BASE_NS::Math::Vec3 expected;
            BASE_NS::Math::Vec3 expectedRangeStart;
            BASE_NS::Math::Vec3 expectedRangeEnd;
            if (progress >= timestamps[3]) {
                expectedRangeStart = keyframes[2];
                expectedRangeEnd = keyframes[3];
                EXPECT_EQ(GetValue(animation.CurrentKeyframeIndex()), 3) << "Frame: " << frame;
            } else if (progress >= timestamps[2]) {
                expectedRangeStart = keyframes[2];
                expectedRangeEnd = keyframes[3];
                EXPECT_EQ(handlerObjects[2]->Called(), 1) << "Frame: " << frame;
                EXPECT_EQ(GetValue(animation.CurrentKeyframeIndex()), 2) << "Frame: " << frame;
            } else if (progress >= timestamps[1]) {
                expectedRangeStart = keyframes[1];
                expectedRangeEnd = keyframes[2];
                EXPECT_EQ(handlerObjects[1]->Called(), 1) << "Frame: " << frame;
                EXPECT_EQ(GetValue(animation.CurrentKeyframeIndex()), 1) << "Frame: " << frame;
            } else if (progress >= timestamps[0]) {
                expectedRangeStart = keyframes[0];
                expectedRangeEnd = keyframes[1];
                EXPECT_EQ(handlerObjects[0]->Called(), 1) << "Frame: " << frame;
                EXPECT_EQ(GetValue(animation.CurrentKeyframeIndex()), 0) << "Frame: " << frame;
            } else {
                // Should not end up here
                FAIL() << "Frame: " << frame;
            }
            EXPECT_GE(value, expectedRangeStart) << "Frame: " << frame;
            EXPECT_LE(value, expectedRangeEnd) << "Frame: " << frame;
            EXPECT_NE(previous, value) << "Frame: " << frame;
            previous = value;
        }
    });

    EXPECT_EQ(steps, (timestamps[3] - timestamps[0]) * 100 + 1);

    EXPECT_EQ(GetValue(animation.Progress()), 1.f);
    EXPECT_EQ(GetValue(animation.CurrentKeyframeIndex()), 3);
    EXPECT_EQ(GetValue(property), keyframes.back());
    EXPECT_FALSE(GetValue(animation.Running()));

    animation.Stop();

    for (const auto& h : handlerObjects) {
        EXPECT_EQ(h->Called(), 1);
    }
    EXPECT_FALSE(GetValue(animation.Running()));

    auto value = GetValue(property);

    animation.Start();
    animation.Step(GetTestClock());

    EXPECT_TRUE(GetValue(animation.Running()));
    EXPECT_EQ(GetValue(animation.Progress()), 0.f);
    EXPECT_EQ(value, GetValue(property)); // Value should not change as our first keyframe is at 0.1

    // Jump past the end
    IncrementClockTime(TimeSpan::Milliseconds(2000));
    animation.Step(GetTestClock());

    EXPECT_EQ(GetValue(animation.Progress()), 1.f);
    EXPECT_EQ(keyframes.back(), GetValue(property));

    UnregisterObjectType<KeyframeHandler>();
}

static BASE_NS::vector<float> trackTimestamps = { 0.1f, 0.5f, 1.f };
static BASE_NS::vector<BASE_NS::Math::Vec3> trackKeyframes = { { 50, 30, 0 }, { 170, 150, 0 }, { 500, 500, 0 } };
static BASE_NS::Math::Vec3 trackInitialValue = { 10, 10, 0 };

TEST_F(TrackAnimationTest, AddKeyframe)
{
    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", trackInitialValue);
    auto added = ConstructProperty<BASE_NS::Math::Vec3>("Prop", { 100, 80, 0 });
    auto addedValue = added->GetValueAny().Clone(true);

    auto animation = META_NS::TrackAnimation<BASE_NS::Math::Vec3>()
                         .Keyframes(trackKeyframes)
                         .Timestamps(trackTimestamps)
                         .Property(property)
                         .Duration(TimeSpan::Milliseconds(1000));

    ASSERT_TRUE(GetValue(animation.Valid()));
    ASSERT_EQ(TimeSpan::Seconds(1.0f), GetValue(animation.TotalDuration()));

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(GetValue(animation.Running()));

    StepAnimations({ animation }, 1, 200, [&animation](uint32_t frame) {
        EXPECT_EQ(GetValue(animation.CurrentKeyframeIndex()), 0) << frame; // 1st keyframe
    });

    // Add a keyframe that is in the middle of the current and next keyframe
    EXPECT_EQ(animation.AddKeyframe(0.3, addedValue), 1);

    StepAnimations({ animation }, 1, 100, [&animation, &property, &added](uint32_t frame) {
        EXPECT_EQ(GetValue(animation.CurrentKeyframeIndex()), 1); // 2nd keyframe that we just added
        EXPECT_EQ(GetValue(property), GetValue(added));
    });
}

TEST_F(TrackAnimationTest, AddKeyframes)
{
    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", trackInitialValue);
    auto added = ConstructProperty<BASE_NS::Math::Vec3>("Prop", { 100, 80, 0 });

    auto animation =
        META_NS::TrackAnimation<BASE_NS::Math::Vec3>().Property(property).Duration(TimeSpan::Milliseconds(1000));

    size_t keyframes = 0;
    for (int i = 0; i < trackTimestamps.size() - 1; i++) {
        added->SetValue(trackKeyframes[i]);
        auto addedValue = added->GetValueAny().Clone(true);
        EXPECT_EQ(animation.AddKeyframe(trackTimestamps[i], addedValue), i);
        EXPECT_EQ(GetValue(animation.Valid()), ++keyframes > 1);
    }

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(GetValue(animation.Running()));

    added->SetValue(trackKeyframes[trackTimestamps.size() - 1]);
    auto addedValue = added->GetValueAny().Clone(true);
    EXPECT_EQ(animation.AddKeyframe(trackTimestamps[trackTimestamps.size() - 1], addedValue), 2);

    EXPECT_EQ(animation.Keyframes()->GetSize(), trackKeyframes.size());
    EXPECT_EQ(animation.Timestamps()->GetSize(), trackTimestamps.size());

    StepAnimations({ animation }, 3, 359, [&animation, &property, &added](uint32_t frame) {
        EXPECT_EQ(GetValue(animation.CurrentKeyframeIndex()), frame - 1) << frame;
    });
}

TEST_F(TrackAnimationTest, RemoveKeyframe)
{
    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", trackInitialValue);
    auto animation = META_NS::TrackAnimation<BASE_NS::Math::Vec3>()
                         .Keyframes(trackKeyframes)
                         .Timestamps(trackTimestamps)
                         .Property(property)
                         .Duration(TimeSpan::Milliseconds(1000));

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(GetValue(animation.Running()));

    StepAnimations({ animation }, 1, 300, [&animation](uint32_t frame) {
        EXPECT_EQ(GetValue(animation.CurrentKeyframeIndex()), 0) << frame; // 1st keyframe
    });

    EXPECT_TRUE(animation.RemoveKeyframe(1));

    EXPECT_EQ(animation.Keyframes()->GetSize(), trackKeyframes.size() - 1);
    EXPECT_EQ(animation.Timestamps()->GetSize(), trackTimestamps.size() - 1);

    StepAnimations({ animation }, 1, 300, [&animation](uint32_t frame) {
        // Originally this would have moved to second keyframe, but since we removed it
        // we should still be in the first keyframe
        EXPECT_EQ(GetValue(animation.CurrentKeyframeIndex()), 0) << frame;
    });
}

TEST_F(TrackAnimationTest, RemoveAllKeyframes)
{
    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", trackInitialValue);
    auto animation = META_NS::TrackAnimation<BASE_NS::Math::Vec3>()
                         .Keyframes(trackKeyframes)
                         .Timestamps(trackTimestamps)
                         .Property(property)
                         .Duration(TimeSpan::Milliseconds(1000));

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(GetValue(animation.Running()));

    animation.RemoveAllKeyframes();
    EXPECT_FALSE(GetValue(animation.Running()));
    EXPECT_FALSE(GetValue(animation.Valid()));
}

TEST_F(TrackAnimationTest, HandlerTest)
{
    RegisterObjectType<KeyframeHandler>();
    static BASE_NS::vector<float> timestamps = { 0.0f, 0.3f, 0.8f, 1.f };
    static BASE_NS::vector<BASE_NS::Math::Vec3> keyframes = { { 50, 30, 0 }, { 100, 80, 0 }, { 170, 150, 0 },
        { 500, 500, 0 } };
    static BASE_NS::Math::Vec3 initialValue = { 10, 10, 0 };

    BASE_NS::vector<ITestKeyframeHandler::Ptr> handlerObjects;
    BASE_NS::vector<IFunction::Ptr> handlers;
    for (int i = 0; i != timestamps.size(); ++i) {
        handlerObjects.push_back(META_NS::GetObjectRegistry().Create<ITestKeyframeHandler>(ClassId::KeyframeHandler));
        handlers.push_back(interface_cast<IMetadata>(handlerObjects.back())->GetFunction("MyFunc"));
    }

    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", initialValue);

    auto animation = META_NS::TrackAnimation<BASE_NS::Math::Vec3>()
                         .Keyframes(keyframes)
                         .Timestamps(timestamps)
                         .Property(property)
                         .Duration(TimeSpan::Milliseconds(1000))
                         .KeyframeHandlers(handlers);

    ASSERT_TRUE(GetValue(animation.Valid()));

    ASSERT_EQ(TimeSpan::Seconds(1.0f), GetValue(animation.TotalDuration()));

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(GetValue(animation.Running()));
    BASE_NS::Math::Vec3 previous = initialValue;

    // Each step steps by 10ms, so 100 steps should cover the whole 1000ms animation
    this->StepAnimations({ animation }, 100, [&](uint32_t frame) {});

    EXPECT_EQ(GetValue(animation.Progress()), 1.f);
    EXPECT_EQ(GetValue(animation.CurrentKeyframeIndex()), 3);
    for (const auto& h : handlerObjects) {
        EXPECT_EQ(h->Called(), 1);
    }
    UnregisterObjectType<KeyframeHandler>();
}

TEST_F(TrackAnimationTest, HandlerTest2)
{
    RegisterObjectType<KeyframeHandler>();
    static BASE_NS::vector<float> timestamps = { 0.3f, 0.4f, 0.6f, 0.7f };
    static BASE_NS::vector<BASE_NS::Math::Vec3> keyframes = { { 50, 30, 0 }, { 100, 80, 0 }, { 170, 150, 0 },
        { 500, 500, 0 } };
    static BASE_NS::Math::Vec3 initialValue = { 10, 10, 0 };

    BASE_NS::vector<ITestKeyframeHandler::Ptr> handlerObjects;
    BASE_NS::vector<IFunction::Ptr> handlers;
    for (int i = 0; i != timestamps.size(); ++i) {
        handlerObjects.push_back(META_NS::GetObjectRegistry().Create<ITestKeyframeHandler>(ClassId::KeyframeHandler));
        handlers.push_back(interface_cast<IMetadata>(handlerObjects.back())->GetFunction("MyFunc"));
    }

    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", initialValue);

    auto animation = META_NS::TrackAnimation<BASE_NS::Math::Vec3>()
                         .Keyframes(keyframes)
                         .Timestamps(timestamps)
                         .Property(property)
                         .Duration(TimeSpan::Milliseconds(1000))
                         .KeyframeHandlers(handlers);

    ASSERT_TRUE(GetValue(animation.Valid()));

    ASSERT_EQ(TimeSpan::Seconds(1.0f), GetValue(animation.TotalDuration()));

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(GetValue(animation.Running()));
    // Each step steps by 10ms, so 100 steps should cover the whole 1000ms animation
    this->StepAnimations({ animation }, 100, [&](uint32_t frame) {});

    EXPECT_EQ(GetValue(animation.Progress()), 1.f);
    EXPECT_EQ(GetValue(animation.CurrentKeyframeIndex()), ITrackAnimation::INVALID_INDEX);
    for (const auto& h : handlerObjects) {
        EXPECT_EQ(h->Called(), 1);
    }
    UnregisterObjectType<KeyframeHandler>();
}

TEST_F(TrackAnimationTest, HandlerTest3)
{
    RegisterObjectType<KeyframeHandler>();
    static BASE_NS::vector<float> timestamps = { 0.0f, 0.4f, 0.6f, 1.0f };
    static BASE_NS::vector<BASE_NS::Math::Vec3> keyframes = { { 50, 30, 0 }, { 100, 80, 0 }, { 170, 150, 0 },
        { 500, 500, 0 } };
    static BASE_NS::Math::Vec3 initialValue = { 10, 10, 0 };

    BASE_NS::vector<ITestKeyframeHandler::Ptr> handlerObjects;
    BASE_NS::vector<IFunction::Ptr> handlers;
    for (int i = 0; i != timestamps.size(); ++i) {
        handlerObjects.push_back(META_NS::GetObjectRegistry().Create<ITestKeyframeHandler>(ClassId::KeyframeHandler));
        handlers.push_back(interface_cast<IMetadata>(handlerObjects.back())->GetFunction("MyFunc"));
    }

    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", initialValue);

    auto animation = META_NS::TrackAnimation<BASE_NS::Math::Vec3>()
                         .Keyframes(keyframes)
                         .Timestamps(timestamps)
                         .Property(property)
                         .Duration(TimeSpan::Milliseconds(1000))
                         .KeyframeHandlers(handlers);

    ASSERT_TRUE(GetValue(animation.Valid()));

    auto loop = META_NS::AnimationModifiers::Loop().LoopCount(3);
    auto attach = interface_pointer_cast<META_NS::IAttach>(animation);
    attach->Attach(loop);

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(GetValue(animation.Running()));
    // Each step steps by 10ms, so 300 steps should cover the whole 1000ms animation 3 times
    this->StepAnimations(
        { animation }, 302, [&](uint32_t frame) {}); // TODO figure out why we need 2 extra frames, 300 should be enough

    EXPECT_EQ(GetValue(animation.Progress()), 1.f);
    EXPECT_EQ(GetValue(animation.CurrentKeyframeIndex()), 3);

    EXPECT_EQ(handlerObjects[0]->Called(), 3);
    EXPECT_EQ(handlerObjects[1]->Called(), 3);
    EXPECT_EQ(handlerObjects[2]->Called(), 3);
    EXPECT_EQ(handlerObjects[3]->Called(), 3);

    UnregisterObjectType<KeyframeHandler>();
}

TEST_F(TrackAnimationTest, KeyframeCurves)
{
    constexpr auto keyframeCount = 4;
    constexpr auto animationMs = 1000;
    BASE_NS::vector<float> timestamps = { 0.0f, 0.4f, 0.6f, 1.0f };
    BASE_NS::vector<BASE_NS::Math::Vec3> keyframes = { { 50, 30, 0 }, { 100, 80, 0 }, { 170, 150, 0 },
        { 500, 500, 0 } };
    BASE_NS::Math::Vec3 initialValue = { 10, 10, 0 };
    BASE_NS::vector<ICurve1D::Ptr> curves = { META_NS::Curves::Easing::InOutCubic(), nullptr,
        META_NS::Curves::Easing::OutBack() };
    constexpr auto propertyName = "PropToAnimate";

    auto property = ConstructProperty<BASE_NS::Math::Vec3>(propertyName, initialValue);

    auto animation = META_NS::TrackAnimation<BASE_NS::Math::Vec3>()
                         .Keyframes(keyframes)
                         .Timestamps(timestamps)
                         .Property(property)
                         .Duration(TimeSpan::Milliseconds(animationMs))
                         .KeyframeCurves(curves);
    animation.AddProperty(property);

    ASSERT_TRUE(GetValue(animation.Valid()));

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(GetValue(animation.Running()));
    const float step = animationMs / 100.f;

    auto checkFrame = [&](const Property<BASE_NS::Math::Vec3>& animated,
                          const META_NS::TrackAnimation<BASE_NS::Math::Vec3>& checkAnim, uint32_t frame) {
        auto linearProgress = (frame * step) / animationMs;
        uint32_t keyframe = 0;
        // Find keyframe were supposed to be in
        for (int i = timestamps.size() - 1; i >= 0; i--) {
            if (timestamps[timestamps.size() - 1] == linearProgress || timestamps[i] <= linearProgress) {
                keyframe = i;
                break;
            }
        }
        EXPECT_EQ(linearProgress, GetValue(checkAnim.Progress())) << "Frame:" << frame;
        EXPECT_EQ(keyframe, GetValue(checkAnim.CurrentKeyframeIndex())) << "Frame:" << frame;

        if (keyframe < timestamps.size() - 1) {
            auto startTs = timestamps[keyframe];
            auto endTs = timestamps[keyframe + 1];
            auto start = keyframes[keyframe];
            auto end = keyframes[keyframe + 1];
            auto trackProgress = (1.f / (endTs - startTs)) * (linearProgress - startTs);
            if (auto curve = curves[keyframe]) {
                trackProgress = curve->Transform(trackProgress);
            }
            auto expected = start + (end - start) * trackProgress;
            EXPECT_EQ(expected, GetValue(animated));
        }
    };

    // Run the animation we just created, checking that every frame proceeds according to expected keyframe/curve
    this->StepAnimations({ animation }, 100, step, [&](uint32_t frame) { checkFrame(property, animation, frame); });

    // Export&import our animation, run it again and check that also imported animation works as expected
    TestSerialiser serializer;
    ASSERT_TRUE(serializer.Export(animation));
    serializer.Dump("file://./test.json");

    META_NS::TrackAnimation<BASE_NS::Math::Vec3> imported;
    ASSERT_TRUE(imported.Initialize(serializer.Import()));
    auto importedProp = imported.Metadata().GetProperty<BASE_NS::Math::Vec3>(propertyName);
    ASSERT_TRUE(importedProp);

    imported.Start();
    imported.Step(this->GetTestClock());
    EXPECT_TRUE(imported.Running()->GetValue());

    this->StepAnimations({ imported }, 100, step, [&](uint32_t frame) { checkFrame(importedProp, imported, frame); });
    imported.RemoveProperty(importedProp);

    animation.RemoveProperty(property);
}

TEST_F(TrackAnimationTest, PropertyOnChanged)
{
    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", trackInitialValue);

    int count = 0;
    property->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] {
        ++count;
        property->GetValue();
    }));

    auto animation = META_NS::TrackAnimation<BASE_NS::Math::Vec3>()
                         .Keyframes(trackKeyframes)
                         .Timestamps(trackTimestamps)
                         .Property(property)
                         .Duration(TimeSpan::Milliseconds(1000));

    ASSERT_TRUE(GetValue(animation.Valid()));
    ASSERT_EQ(TimeSpan::Seconds(1.0f), GetValue(animation.TotalDuration()));

    animation.Start();
    StepAnimations({ animation }, 102, [&animation, &property](uint32_t frame) {

    });
    EXPECT_FALSE(GetValue(animation.Running()));
    EXPECT_TRUE(count >= 50) << count;
}

TEST_F(TrackAnimationTest, SeekingPaused)
{
    auto property = ConstructProperty<int>("Prop");

    static BASE_NS::vector<float> trackTimestamps = { 0.0f, 1.f };
    static BASE_NS::vector<int> trackKeyframes = { 0, 10 };
    auto animation = META_NS::TrackAnimation<int>()
                         .Keyframes(trackKeyframes)
                         .Timestamps(trackTimestamps)
                         .Property(property)
                         .Duration(TimeSpan::Milliseconds(1000));

    ASSERT_TRUE(GetValue(animation.Valid()));
    ASSERT_EQ(TimeSpan::Seconds(1.0f), GetValue(animation.TotalDuration()));

    animation.Start();
    StepAnimations({ animation }, 10, [](uint32_t frame) {});
    animation.Pause();

    int count = 0;
    property->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] {
        ++count;
        EXPECT_EQ(property->GetValue(), count);
    }));

    // should do nothing
    StepAnimations({ animation }, 10, [](uint32_t frame) {});

    animation.Seek(0.1f);

    // should do nothing
    StepAnimations({ animation }, 10, [](uint32_t frame) {});

    EXPECT_EQ(property->GetValue(), 1);
    EXPECT_EQ(count, 1);
    animation.Seek(0.2f);
    EXPECT_EQ(property->GetValue(), 2);
    EXPECT_EQ(count, 2);
    animation.Seek(0.3f);
    EXPECT_EQ(property->GetValue(), 3);
    EXPECT_EQ(count, 3);
}

} // namespace Test

META_END_NAMESPACE()
