/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#include <functional>

#include <gtest/gtest.h>

#include <meta/api/make_callback.h>
#include <meta/api/object.h>
#include <meta/base/capture.h>
#include <meta/ext/attachment/attachment.h>
#include <meta/interface/intf_object_registry.h>

#include "testing_objects.h"
#include "util.h"

META_BEGIN_NAMESPACE()
namespace Test {

TEST(CaptureBindTest, LambdaAutoLocksCapturedSharedPtr)
{
    {
        BASE_NS::shared_ptr<int> sp(new int { 6 });
        auto f = CaptureSafe([](auto p) { *p = 1; }, sp);

        EXPECT_EQ(*sp, 6);
        EXPECT_NO_THROW(f());
        EXPECT_EQ(*sp, 1);
    }

    {
        const BASE_NS::shared_ptr<int> sp(new int { 6 });
        auto f = CaptureSafe([](auto p) { *p = 1; }, sp);

        EXPECT_EQ(*sp, 6);
        EXPECT_NO_THROW(f());
        EXPECT_EQ(*sp, 1);
    }
}

TEST(CaptureBindTest, LambdaAutoLocksCapturedWeakPtr)
{
    {
        BASE_NS::shared_ptr<int> sp(new int { 6 });
        auto wp = BASE_NS::weak_ptr(sp);
        auto f = CaptureSafe([](auto p) { *p = 1; }, wp);

        EXPECT_EQ(*sp, 6);
        EXPECT_NO_THROW(f());
        EXPECT_EQ(*sp, 1);
    }

    {
        BASE_NS::shared_ptr<int> sp(new int { 6 });
        const auto wp = BASE_NS::weak_ptr(sp);
        auto f = CaptureSafe([](auto p) { *p = 1; }, wp);

        EXPECT_EQ(*sp, 6);
        EXPECT_NO_THROW(f());
        EXPECT_EQ(*sp, 1);
    }
}

TEST(CaptureBindTest, LambdaNotThrowsWhenCapturedSharedPointerIsReleased)
{
    {
        BASE_NS::shared_ptr<int> p(new int {});

        auto f = CaptureSafe([](auto p) { *p = 1; }, p);
        p.reset();

        EXPECT_NO_THROW(f());
    }
}

TEST(CaptureBindTest, LambdaNotThrowsWhenCapturedWeakPointerPointsToInvalidResource)
{
    {
        BASE_NS::shared_ptr<int> sp(new int {});

        auto f = CaptureSafe([](auto p) { *p = 1; }, BASE_NS::weak_ptr(sp));
        sp.reset();

        EXPECT_NO_THROW(f());
    }

    {
        BASE_NS::shared_ptr<int> sp(new int {});
        BASE_NS::weak_ptr<int> wp = BASE_NS::weak_ptr(sp);

        auto f = CaptureSafe([](auto p) { *p = 1; }, wp);
        sp.reset();

        EXPECT_NO_THROW(f());
    }
}

TEST(CaptureBindTest, SimpleLambda)
{
    BASE_NS::shared_ptr<int> p(new int {});

    {
        auto f = Capture([](auto pp) { *pp = 1; }, p);
        f();
        EXPECT_EQ(*p, 1);
    }
    {
        auto f = Capture([](auto p, int v) { *p = v; }, p);
        f(2);
        EXPECT_EQ(*p, 2);
    }
    {
        auto f = Capture([]() { return true; });
        EXPECT_TRUE(f());
    }
    {
        auto f = Capture([](int v) { v = 1; });
        int i = 0;
        f(i);
        EXPECT_EQ(i, 0);
    }
    {
        auto f = Capture([](int& v) { v = 1; });
        int i = 0;
        f(i);
        EXPECT_EQ(i, 1);
    }
    {
        BASE_NS::shared_ptr<int> p(new int {});
        auto f = Capture([](auto p) { return (bool)p; }, p);
        EXPECT_TRUE(f());
        p.reset();
        EXPECT_FALSE(f());
    }
    {
        std::function<bool()> func;
        {
            const BASE_NS::shared_ptr<int> p(new int {});
            func = Capture([](auto p) { return (bool)p; }, p);
            EXPECT_TRUE(func());
        }
        EXPECT_FALSE(func());
    }
    {
        BASE_NS::shared_ptr<int> p(new int {});
        auto other = p;
        auto f = Capture([](auto p) { return (bool)p; }, BASE_NS::move(p));
        EXPECT_TRUE(f());
        other.reset();
        EXPECT_FALSE(f());
    }
}

TEST(CaptureBindTest, MakeCallback)
{
    auto& registry = GetObjectRegistry();
    auto p = META_NS::ConstructProperty<int>(registry, "P1", 0);

    BASE_NS::shared_ptr<int> count(new int {});
    p->OnChanged()->AddHandler(MakeCallback<IOnChanged>(
        [](auto i) {
            if (i) {
                ++*i;
            }
        },
        count));

    p->SetValue(2);
    EXPECT_EQ(*count, 1);
}

TEST(CaptureBindTest, MakeCallable)
{
    auto& registry = GetObjectRegistry();
    auto c = CreateTestContainer<IContainer>("Test");

    BASE_NS::shared_ptr<int> count(new int {});
    c->OnContainerChanged()->AddHandler(MakeCallback<IOnChildChanged>(
        [](auto i, const ChildChangedInfo& info) {
            if (i) {
                ++*i;
            }
        },
        count));

    c->Add(CreateInstance(ClassId::Object));
    EXPECT_EQ(*count, 1);
}

namespace {
META_REGISTER_CLASS(TestAttachment, "d4e854a2-b169-4b90-ae31-76e78cdf07b0", META_NS::ObjectCategoryBits::APPLICATION)

class TestAttachment : public META_NS::AttachmentFwd {
    META_OBJECT(TestAttachment, ClassId::TestAttachment, AttachmentFwd)
    bool AttachTo(const META_NS::IAttach::Ptr& target, const IObject::Ptr& dataContext) override
    {
        return true;
    }
    bool DetachFrom(const META_NS::IAttach::Ptr& target) override
    {
        return true;
    }
};
} // namespace

TEST(CaptureBindTest, IObject)
{
    RegisterObjectType<TestAttachment>();
    IObject::Ptr obj = CreateInstance(META_NS::ClassId::Object);
    auto func = Capture(
        [](IObject::Ptr p) {
            if (auto a = interface_cast<IAttach>(p)) {
                a->Attach(GetObjectRegistry().Create<IAttachment>(ClassId::TestAttachment));
                return true;
            }
            return false;
        },
        obj);

    EXPECT_TRUE(func());
    EXPECT_EQ(interface_cast<IAttach>(obj)->GetAttachments().size(), 1);
    obj.reset();
    EXPECT_FALSE(func());
    UnregisterObjectType<TestAttachment>();
}

} // namespace Test
META_END_NAMESPACE()
