/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <test_framework.h>

#include <base/math/vector.h>

#include <meta/api/property/property_event_handler.h>
#include <meta/interface/alias/intf_alias_registry.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/detail/multi_type_any.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/construct_property.h>

#include "helpers/animation_test_base.h"

META_BEGIN_NAMESPACE()
namespace UTest {

namespace {
bool Contains(const BASE_NS::vector<BASE_NS::string>& names, BASE_NS::string_view name)
{
    for (auto& n : names) {
        if (n == name) {
            return true;
        }
    }
    return false;
}

bool Contains(const BASE_NS::vector<AliasInfo>& aliases, BASE_NS::string_view name)
{
    for (auto& a : aliases) {
        if (a.name == name) {
            return true;
        }
    }
    return false;
}

const AliasInfo* Find(const BASE_NS::vector<AliasInfo>& aliases, BASE_NS::string_view name)
{
    for (auto& a : aliases) {
        if (a.name == name) {
            return &a;
        }
    }
    return nullptr;
}

bool Contains(const BASE_NS::vector<IProperty::Ptr>& props, const IProperty::Ptr& prop)
{
    for (auto& p : props) {
        if (p == prop) {
            return true;
        }
    }
    return false;
}

// Records what a registry reports, and reads back through it while it is being told — the interface
// promises a listener may, since the registry holds no lock while calling one.
class RecordingListener : public IntroduceInterfaces<IAliasListener> {
public:
    struct Change {
        AliasChangeType type;
        BASE_NS::string name;
        bool scoped{};
        // What the registry answered when asked from inside the callback.
        TypeId typeAtCall;
    };

    void OnAliasChanged(const AliasChangedInfo& info) override
    {
        for (auto& name : info.names) {
            changes.push_back(Change{
                info.type, name, bool(info.scope), registry ? registry->GetAliasType(name, info.scope) : TypeId{}});
        }
    }

    size_t Count(AliasChangeType type, BASE_NS::string_view name) const
    {
        size_t count = 0;
        for (auto& c : changes) {
            if (c.type == type && c.name == name) {
                ++count;
            }
        }
        return count;
    }

    IAliasRegistry* registry{};
    BASE_NS::vector<Change> changes;
};

// A double that can also be read as a float. This is the shape a document's bare number arrives in —
// the importer parses one to whatever type fits and lets the narrower ones convert — and so the reason
// an inherited value has to be converted rather than matched on its type id.
class LooseDouble : public MultiTypeAny<double, StaticCastConv, float> {
public:
    explicit LooseDouble(double v = {}) : value_(v)
    {}

protected:
    IAny::Ptr RawClone(const AnyCloneOptions& options) const override
    {
        return IAny::Ptr(new LooseDouble{options.value == CloneValueType::COPY_VALUE ? value_ : 0.0});
    }

private:
    AnyReturnValue InternalSetValue(const double& value) override
    {
        value_ = value;
        return AnyReturn::SUCCESS;
    }
    const double& InternalGetValue() const override
    {
        return value_;
    }

    double value_{};
};
}  // namespace

// Derives from the animation test base only for its manual clock: a transition is stepped by the
// animation controller, so advancing one means advancing that.
class API_AliasRegistryTest : public API_AnimationTestBase {
public:
    IAliasRegistry::Ptr MakeRegistry()
    {
        return GetObjectRegistry().Create<IAliasRegistry>(ClassId::AliasRegistry);
    }

    // A scope is only ever compared by identity, so any object will do; a scene is what a real caller
    // passes.
    AliasScopePtr MakeScope()
    {
        return interface_pointer_cast<CORE_NS::IInterface>(GetObjectRegistry().Create<IObject>(ClassId::Object));
    }
};

/**
 * @tc.name: BindBeforeDefine
 * @tc.desc: Binding before the alias is defined yields the default-constructed value; a later Define
 *           drives the bound consumer.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, BindBeforeDefine, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    ASSERT_TRUE(provider);
    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "x"));
    EXPECT_FLOAT_EQ(c->GetValue(), 0.f);

    EXPECT_TRUE(provider->Define("x", *ConstructAny<float>(5.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 5.f);
}

/**
 * @tc.name: FanOutAndRedefine
 * @tc.desc: Define fans out to every bound consumer; redefining updates them all and notifies.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, FanOutAndRedefine, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto c1 = ConstructProperty<float>("c1", 0.f);
    auto c2 = ConstructProperty<float>("c2", 0.f);
    auto c3 = ConstructProperty<float>("c3", 0.f);
    ASSERT_TRUE(provider->Bind(c1.GetProperty(), "v"));
    ASSERT_TRUE(provider->Bind(c2.GetProperty(), "v"));
    ASSERT_TRUE(provider->Bind(c3.GetProperty(), "v"));

    int notified = 0;
    PropertyChangedEventHandler handler;
    handler.Subscribe(c1.GetProperty(), [&notified] { ++notified; });

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(1.f)));
    EXPECT_FLOAT_EQ(c1->GetValue(), 1.f);
    EXPECT_FLOAT_EQ(c2->GetValue(), 1.f);
    EXPECT_FLOAT_EQ(c3->GetValue(), 1.f);

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(2.f)));
    EXPECT_FLOAT_EQ(c1->GetValue(), 2.f);
    EXPECT_FLOAT_EQ(c2->GetValue(), 2.f);
    EXPECT_FLOAT_EQ(c3->GetValue(), 2.f);

    EXPECT_GE(notified, 2);
}

/**
 * @tc.name: DefaultAndUndefine
 * @tc.desc: A consumer falls back to the alias default; Undefine returns the cell to that default
 *           while keeping the binding.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, DefaultAndUndefine, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    ASSERT_TRUE(provider->SetDefault("v", ConstructAny<float>(9.f)));
    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v"));
    EXPECT_FLOAT_EQ(c->GetValue(), 9.f);

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(3.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 3.f);

    EXPECT_TRUE(provider->Undefine("v"));
    EXPECT_FLOAT_EQ(c->GetValue(), 9.f);
    EXPECT_TRUE(Contains(provider->GetAliases(), "v"));
}

/**
 * @tc.name: SettingAValueReplacesTheBinding
 * @tc.desc: A binding is a value, so setting one replaces it: the property stops following the alias
 *           and keeps what it was given. The registry must stop counting it as a consumer too.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, SettingAValueReplacesTheBinding, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v"));
    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(1.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);
    EXPECT_EQ(provider->GetConsumerCount("v"), 1u);

    // Bind::SetValue refuses, so the stack drops the bind and writes beneath it.
    c->SetValue(5.f);
    EXPECT_FLOAT_EQ(c->GetValue(), 5.f);

    // The alias no longer decides this property.
    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(2.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 5.f);
    // And the registry no longer holds it as one of the alias's consumers.
    EXPECT_EQ(provider->GetConsumerCount("v"), 0u);
    EXPECT_TRUE(provider->GetConsumers("v").empty());
}

/**
 * @tc.name: TypeConflictRejected
 * @tc.desc: A mismatched Bind/Define is rejected and leaves the established alias untouched.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, TypeConflictRejected, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    EXPECT_TRUE(provider->Define("t", *ConstructAny<float>(1.f)));

    auto vec = ConstructProperty<BASE_NS::Math::Vec3>("vec", {});
    EXPECT_FALSE(provider->Bind(vec.GetProperty(), "t"));
    EXPECT_FALSE(provider->Define("t", *ConstructAny<BASE_NS::Math::Vec3>({1.f, 2.f, 3.f})));

    // The original float alias is intact: a matching consumer still sees the original value.
    auto f = ConstructProperty<float>("f", 0.f);
    ASSERT_TRUE(provider->Bind(f.GetProperty(), "t"));
    EXPECT_FLOAT_EQ(f->GetValue(), 1.f);
}

/**
 * @tc.name: RemoveSelfHeals
 * @tc.desc: Removing an alias destroys its cell; bound consumers fall back to their own value and a
 *           later same-name Define does not recapture them.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, RemoveSelfHeals, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto c = ConstructProperty<float>("c", 7.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "r"));
    EXPECT_TRUE(provider->Define("r", *ConstructAny<float>(1.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);

    EXPECT_TRUE(provider->Remove("r"));
    EXPECT_FALSE(Contains(provider->GetAliases(), "r"));
    EXPECT_FLOAT_EQ(c->GetValue(), 7.f);

    // A fresh same-name cell must not affect the already-unbound consumer.
    EXPECT_TRUE(provider->Define("r", *ConstructAny<float>(2.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 7.f);
}

/**
 * @tc.name: RemoveAfterConsumerDestroyed
 * @tc.desc: Removing an alias after its only consumer has been destroyed is safe.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, RemoveAfterConsumerDestroyed, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    {
        auto tmp = ConstructProperty<float>("tmp", 0.f);
        ASSERT_TRUE(provider->Bind(tmp.GetProperty(), "d"));
        EXPECT_TRUE(provider->Define("d", *ConstructAny<float>(1.f)));
    }
    EXPECT_TRUE(provider->Remove("d"));
    EXPECT_FALSE(Contains(provider->GetAliases(), "d"));
}

/**
 * @tc.name: ConsumersPrunedOnRebind
 * @tc.desc: A consumer slot left behind by a destroyed property (e.g. a node template re-instantiated
 *           onto a fresh property) is pruned on the next Bind, so the retained-slot count stays bounded
 *           to live bindings instead of growing per rebind.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, ConsumersPrunedOnRebind, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    ASSERT_TRUE(provider);
    EXPECT_EQ(provider->GetConsumerCount("nonexistent"), 0u);

    {
        auto tmp = ConstructProperty<float>("tmp", 0.f);
        ASSERT_TRUE(provider->Bind(tmp.GetProperty(), "x"));
        EXPECT_EQ(provider->GetConsumerCount("x"), 1u);
    }  // tmp destroyed -> its consumer slot is now an expired weak_ptr

    // Rebinding onto a fresh property must prune the dead slot, not accumulate it.
    auto live = ConstructProperty<float>("live", 0.f);
    ASSERT_TRUE(provider->Bind(live.GetProperty(), "x"));
    EXPECT_EQ(provider->GetConsumerCount("x"), 1u);
}

/**
 * @tc.name: ConsumerCountIgnoresDestroyedConsumers
 * @tc.desc: The consumer count is over live bindings, so a destroyed consumer stops counting at once
 *           even though its slot is only reclaimed lazily; the surviving consumer keeps following.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, ConsumerCountIgnoresDestroyedConsumers, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto live = ConstructProperty<float>("live", 0.f);
    ASSERT_TRUE(provider->Bind(live.GetProperty(), "y"));
    {
        auto tmp = ConstructProperty<float>("tmp", 0.f);
        ASSERT_TRUE(provider->Bind(tmp.GetProperty(), "y"));
        EXPECT_EQ(provider->GetConsumerCount("y"), 2u);
    }  // tmp destroyed; when its slot is reclaimed is an implementation detail.

    EXPECT_EQ(provider->GetConsumerCount("y"), 1u);
    EXPECT_TRUE(provider->Define("y", *ConstructAny<float>(1.f)));
    EXPECT_EQ(provider->GetConsumerCount("y"), 1u);
    EXPECT_FLOAT_EQ(live->GetValue(), 1.f);
}

/**
 * @tc.name: ConsumersAnswerWhichPropertiesFollowAnAlias
 * @tc.desc: GetConsumers hands back the live consumer properties themselves, so tooling can invert
 *           them over GetNames and answer, for a property in hand, which alias it follows. Destroyed
 *           consumers drop out the way GetConsumerCount stops counting them, and an alias nobody has
 *           established answers empty rather than failing.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, ConsumersAnswerWhichPropertiesFollowAnAlias, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    ASSERT_TRUE(provider);
    EXPECT_TRUE(provider->GetConsumers("nonexistent").empty());

    auto first = ConstructProperty<float>("first", 0.f);
    auto second = ConstructProperty<float>("second", 0.f);
    ASSERT_TRUE(provider->Bind(first.GetProperty(), "c"));
    ASSERT_TRUE(provider->Bind(second.GetProperty(), "c"));

    auto consumers = provider->GetConsumers("c");
    ASSERT_EQ(consumers.size(), 2u);
    EXPECT_TRUE(Contains(consumers, first.GetProperty()));
    EXPECT_TRUE(Contains(consumers, second.GetProperty()));

    // Rebinding one elsewhere moves it, since a property follows at most one alias.
    ASSERT_TRUE(provider->Bind(second.GetProperty(), "d"));
    consumers = provider->GetConsumers("c");
    ASSERT_EQ(consumers.size(), 1u);
    EXPECT_EQ(consumers[0], first.GetProperty());
    ASSERT_EQ(provider->GetConsumers("d").size(), 1u);

    {
        auto tmp = ConstructProperty<float>("tmp", 0.f);
        ASSERT_TRUE(provider->Bind(tmp.GetProperty(), "c"));
        EXPECT_EQ(provider->GetConsumers("c").size(), 2u);
    }  // tmp destroyed; its slot is reclaimed lazily but it stops being a consumer at once.
    ASSERT_EQ(provider->GetConsumers("c").size(), 1u);
    EXPECT_EQ(provider->GetConsumers("c")[0], first.GetProperty());
}

/**
 * @tc.name: RebindDetachesFromPreviousAlias
 * @tc.desc: A property follows at most one alias: rebinding it to another name detaches it from the
 *           first, which then no longer counts it as a consumer nor drives it. Rebinding to the same
 *           name does not accumulate slots either.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, RebindDetachesFromPreviousAlias, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "a"));
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "a"));
    EXPECT_EQ(provider->GetConsumerCount("a"), 1u);

    ASSERT_TRUE(provider->Bind(c.GetProperty(), "b"));
    EXPECT_EQ(provider->GetConsumerCount("a"), 0u);
    EXPECT_EQ(provider->GetConsumerCount("b"), 1u);

    EXPECT_TRUE(provider->Define("b", *ConstructAny<float>(2.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 2.f);

    // The alias it no longer follows must not drive it.
    EXPECT_TRUE(provider->Define("a", *ConstructAny<float>(5.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 2.f);
}

/**
 * @tc.name: GetAlias
 * @tc.desc: GetAlias returns the cell property for an established alias (exposing its value/type) and
 *           null for an unknown name; it does not create the alias.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, GetAlias, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    ASSERT_TRUE(provider);
    EXPECT_FALSE(provider->GetAlias("x"));

    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "x"));
    auto cell = provider->GetAlias("x");
    ASSERT_TRUE(cell);

    EXPECT_TRUE(provider->Define("x", *ConstructAny<float>(3.f)));
    auto cell2 = provider->GetAlias("x");
    ASSERT_TRUE(cell2);
    // The returned cell is the alias source; it now carries the defined value that drives consumers.
    Property<float> typedCell{cell2};
    ASSERT_TRUE(typedCell);
    EXPECT_FLOAT_EQ(typedCell->GetValue(), 3.f);
    EXPECT_FLOAT_EQ(c->GetValue(), 3.f);

    EXPECT_FALSE(provider->GetAlias("y"));
}

/**
 * @tc.name: UnbindDropsOneConsumer
 * @tc.desc: Unbind detaches a single consumer — it falls back to its own value and stops following —
 *           while the alias and its other consumers are untouched. Unbinding something that follows no
 *           alias reports false.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, UnbindDropsOneConsumer, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto c1 = ConstructProperty<float>("c1", 7.f);
    auto c2 = ConstructProperty<float>("c2", 0.f);
    ASSERT_TRUE(provider->Bind(c1.GetProperty(), "u"));
    ASSERT_TRUE(provider->Bind(c2.GetProperty(), "u"));
    EXPECT_TRUE(provider->Define("u", *ConstructAny<float>(1.f)));
    EXPECT_FLOAT_EQ(c1->GetValue(), 1.f);

    EXPECT_TRUE(provider->Unbind(c1.GetProperty()));
    EXPECT_FLOAT_EQ(c1->GetValue(), 7.f);
    EXPECT_EQ(provider->GetConsumerCount("u"), 1u);
    EXPECT_TRUE(Contains(provider->GetAliases(), "u"));

    // The alias keeps driving everyone still bound, and never the one that left.
    EXPECT_TRUE(provider->Define("u", *ConstructAny<float>(2.f)));
    EXPECT_FLOAT_EQ(c2->GetValue(), 2.f);
    EXPECT_FLOAT_EQ(c1->GetValue(), 7.f);

    EXPECT_FALSE(provider->Unbind(c1.GetProperty()));
    EXPECT_FALSE(provider->Unbind(nullptr));
}

/**
 * @tc.name: LateTypedCellNarrowedOnFirstBind
 * @tc.desc: An alias defined from an object whose concrete type is not known yet (an erased interface
 *           pointer, as a resolved resource arrives) stays late-typed: a typed pointer consumer may
 *           still bind to it and receives the object. That first concrete bind fixes the type, after
 *           which mismatched references are rejected again.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, LateTypedCellNarrowedOnFirstBind, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto obj = GetObjectRegistry().Create<IObject>(ClassId::Object);
    ASSERT_TRUE(obj);

    SharedPtrIInterface erased = interface_pointer_cast<CORE_NS::IInterface>(obj);
    EXPECT_TRUE(provider->Define("late", *ConstructAny<SharedPtrIInterface>(erased)));

    // A non-pointer consumer can never carry an interface pointer, so it is rejected even now.
    auto f = ConstructProperty<float>("f", 0.f);
    EXPECT_FALSE(provider->Bind(f.GetProperty(), "late"));

    auto p = ConstructProperty<IObject::Ptr>("p", nullptr);
    ASSERT_TRUE(provider->Bind(p.GetProperty(), "late"));
    EXPECT_EQ(p->GetValue(), obj);

    // The cell is concrete from here on: it takes another IObject::Ptr, but no longer a float.
    auto p2 = ConstructProperty<IObject::Ptr>("p2", nullptr);
    EXPECT_TRUE(provider->Bind(p2.GetProperty(), "late"));
    EXPECT_EQ(p2->GetValue(), obj);
    EXPECT_FALSE(provider->Bind(f.GetProperty(), "late"));
}

/**
 * @tc.name: LateTypedCellCarriesConsumersOverNarrowing
 * @tc.desc: Narrowing keeps consumers that bound while the cell was still erased: they end up on the
 *           new cell and keep following later Defines.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, LateTypedCellCarriesConsumersOverNarrowing, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto first = GetObjectRegistry().Create<IObject>(ClassId::Object);
    auto second = GetObjectRegistry().Create<IObject>(ClassId::Object);
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);

    SharedPtrIInterface erased = interface_pointer_cast<CORE_NS::IInterface>(first);
    EXPECT_TRUE(provider->Define("late", *ConstructAny<SharedPtrIInterface>(erased)));

    // An erased consumer binds while the cell is still late-typed, a concrete one narrows it after.
    auto anyPtr = ConstructProperty<SharedPtrIInterface>("anyPtr", nullptr);
    ASSERT_TRUE(provider->Bind(anyPtr.GetProperty(), "late"));
    auto typed = ConstructProperty<IObject::Ptr>("typed", nullptr);
    ASSERT_TRUE(provider->Bind(typed.GetProperty(), "late"));
    EXPECT_EQ(provider->GetConsumerCount("late"), 2u);

    EXPECT_TRUE(provider->Define("late", *ConstructAny<IObject::Ptr>(second)));
    EXPECT_EQ(typed->GetValue(), second);
    EXPECT_EQ(anyPtr->GetValue(), interface_pointer_cast<CORE_NS::IInterface>(second));
}
/**
 * @tc.name: GroupDepthDecidesValue
 * @tc.desc: A nested group overrides the group it is nested in, whichever order they are applied in.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, GroupDepthDecidesValue, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v"));

    auto accent = provider->GetGroup("theme/accent");
    ASSERT_TRUE(accent);
    EXPECT_TRUE(accent->Define("v", *ConstructAny<float>(2.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 2.f);

    // Applied second, but shallower, so it does not win.
    auto theme = provider->GetGroup("theme");
    ASSERT_TRUE(theme);
    EXPECT_TRUE(theme->Define("v", *ConstructAny<float>(1.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 2.f);

    EXPECT_TRUE(provider->RemoveGroup("theme/accent"));
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);
}

/**
 * @tc.name: UngroupedDefineOverridesEveryGroup
 * @tc.desc: Define sits above the groups, and Undefine drops back onto the topmost of them.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, UngroupedDefineOverridesEveryGroup, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    ASSERT_TRUE(provider->SetDefault("v", ConstructAny<float>(9.f)));
    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v"));

    ASSERT_TRUE(provider->GetGroup("theme")->Define("v", *ConstructAny<float>(1.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(5.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 5.f);

    // A group cannot displace it, however deep.
    ASSERT_TRUE(provider->GetGroup("theme/accent")->Define("v", *ConstructAny<float>(2.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 5.f);

    EXPECT_TRUE(provider->Undefine("v"));
    EXPECT_FLOAT_EQ(c->GetValue(), 2.f);
    EXPECT_TRUE(provider->RemoveGroup("theme"));
    EXPECT_FLOAT_EQ(c->GetValue(), 9.f);
}

/**
 * @tc.name: GroupSetReleasesWhatItDrops
 * @tc.desc: Applying a value set over a group's previous one leaves nothing of the first behind.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, GroupSetReleasesWhatItDrops, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto shared = ConstructProperty<float>("shared", 0.f);
    auto dropped = ConstructProperty<float>("dropped", 0.f);
    ASSERT_TRUE(provider->Bind(shared.GetProperty(), "a"));
    ASSERT_TRUE(provider->Bind(dropped.GetProperty(), "b"));

    auto theme = provider->GetGroup("theme");
    ASSERT_TRUE(theme);
    const AliasEntry first[] = {{"a", ConstructAny<float>(1.f)}, {"b", ConstructAny<float>(2.f)}};
    EXPECT_TRUE(theme->Set(first));
    EXPECT_FLOAT_EQ(shared->GetValue(), 1.f);
    EXPECT_FLOAT_EQ(dropped->GetValue(), 2.f);

    // The second set says nothing about "b", so the group stops deciding it.
    const AliasEntry second[] = {{"a", ConstructAny<float>(3.f)}};
    EXPECT_TRUE(theme->Set(second));
    EXPECT_FLOAT_EQ(shared->GetValue(), 3.f);
    EXPECT_FLOAT_EQ(dropped->GetValue(), 0.f);
    EXPECT_FALSE(Contains(theme->GetNames(), "b"));
}

/**
 * @tc.name: MaskBlocksLowerGroup
 * @tc.desc: A null entry masks the groups below it, and does so whether or not the masked group was
 *           applied first.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, MaskBlocksLowerGroup, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v"));

    // The mask lands before the alias has any value at all.
    ASSERT_TRUE(provider->GetGroup("theme/accent")->Mask("v"));
    ASSERT_TRUE(provider->GetGroup("theme")->Define("v", *ConstructAny<float>(1.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 0.f);

    EXPECT_TRUE(provider->RemoveGroup("theme/accent"));
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);
}

/**
 * @tc.name: RemoveGroupTakesNestedGroups
 * @tc.desc: Removing a group removes everything nested under it, and only that.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, RemoveGroupTakesNestedGroups, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    ASSERT_TRUE(provider->GetGroup("theme")->Define("a", *ConstructAny<float>(1.f)));
    ASSERT_TRUE(provider->GetGroup("theme/accent")->Define("b", *ConstructAny<float>(2.f)));
    // Shares a prefix but not a segment boundary, so it is a different subtree.
    ASSERT_TRUE(provider->GetGroup("themes")->Define("c", *ConstructAny<float>(3.f)));

    auto groups = provider->GetContributingGroups();
    EXPECT_EQ(groups.size(), 3U);
    EXPECT_TRUE(Contains(groups, "theme/accent"));

    EXPECT_TRUE(provider->RemoveGroup("theme"));
    groups = provider->GetContributingGroups();
    EXPECT_EQ(groups.size(), 1U);
    EXPECT_TRUE(Contains(groups, "themes"));
    EXPECT_FALSE(provider->RemoveGroup("theme"));
}

/**
 * @tc.name: GroupHandleAddressesLayerNotState
 * @tc.desc: A group is its path: two handles on one path are interchangeable, and a handle kept across
 *           a removal simply addresses an empty layer.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, GroupHandleAddressesLayerNotState, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v"));

    auto one = provider->GetGroup("theme");
    auto two = provider->GetGroup("theme");
    ASSERT_TRUE(one && two);
    EXPECT_EQ(one->GetPath(), two->GetPath());
    ASSERT_TRUE(one->Define("v", *ConstructAny<float>(1.f)));
    EXPECT_TRUE(Contains(two->GetNames(), "v"));

    EXPECT_FALSE(provider->FindGroup("absent"));
    EXPECT_TRUE(provider->FindGroup("theme"));

    two->Clear();
    EXPECT_FLOAT_EQ(c->GetValue(), 0.f);
    EXPECT_FALSE(provider->FindGroup("theme"));
    // Still usable: it addresses the path, and the layer comes back with the next contribution.
    ASSERT_TRUE(one->Define("v", *ConstructAny<float>(4.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 4.f);
}

/**
 * @tc.name: UngroupedLayerIsQueryable
 * @tc.desc: What Define set can be enumerated and released as a group, while staying above every group
 *           and out of the group listing.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, UngroupedLayerIsQueryable, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v"));
    ASSERT_TRUE(provider->GetGroup("theme")->Define("v", *ConstructAny<float>(1.f)));
    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(5.f)));

    auto ungrouped = provider->GetUngrouped();
    ASSERT_TRUE(ungrouped);
    EXPECT_TRUE(ungrouped->GetPath().empty());
    EXPECT_TRUE(Contains(ungrouped->GetNames(), "v"));
    // It is the values Define set, not the theme's.
    EXPECT_FALSE(Contains(provider->GetGroup("theme")->GetNames(), "other"));

    // Not addressable as a path, and not one of the groups.
    EXPECT_FALSE(Contains(provider->GetContributingGroups(), ""));
    EXPECT_FALSE(provider->FindGroup(""));
    EXPECT_FALSE(provider->RemoveGroup(""));

    // Releasing it is Undefine for everything it holds, so the topmost group takes over.
    ungrouped->Clear();
    EXPECT_TRUE(ungrouped->GetNames().empty());
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);
}

/**
 * @tc.name: MalformedGroupPathRejected
 * @tc.desc: A path is a non-empty sequence of non-empty segments.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, MalformedGroupPathRejected, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    EXPECT_FALSE(provider->GetGroup(""));
    EXPECT_FALSE(provider->GetGroup("/theme"));
    EXPECT_FALSE(provider->GetGroup("theme/"));
    EXPECT_FALSE(provider->GetGroup("theme//accent"));
    EXPECT_TRUE(provider->GetGroup("theme/accent/warm"));
}

/**
 * @tc.name: GroupTransitionEasesTheChange
 * @tc.desc: A transition set on a group eases every value applied there. The first value an alias
 *           receives still appears at once — there is nothing to move from — and a later one moves over
 *           the stated duration, landing exactly on its target.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, GroupTransitionEasesTheChange, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v"));

    auto theme = provider->GetGroup("theme");
    ASSERT_TRUE(theme);
    theme->SetTransition(TransitionSpec{TimeSpan::Milliseconds(500), nullptr});

    // Nothing to ease from, so this lands immediately however long the transition says.
    EXPECT_TRUE(theme->Define("v", *ConstructAny<float>(1.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);

    // The next change eases: it has not moved at all until the clock does.
    EXPECT_TRUE(theme->Define("v", *ConstructAny<float>(2.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);

    Update(TimeSpan::Milliseconds(0));
    Update(TimeSpan::Milliseconds(250));
    EXPECT_NEAR(c->GetValue(), 1.5f, 0.01f);

    // Landing writes the target itself rather than the last interpolated step.
    Update(TimeSpan::Milliseconds(250));
    EXPECT_FLOAT_EQ(c->GetValue(), 2.f);
}

/**
 * @tc.name: StatedDefaultIsAValueToTransitionFrom
 * @tc.desc: A default someone stated is a value the alias holds, so the first value applied over it
 *           eases rather than landing at once — unlike the type's own zero, which is nobody's design.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, StatedDefaultIsAValueToTransitionFrom, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    ASSERT_TRUE(provider->SetDefault("v", ConstructAny<float>(1.f)));
    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v"));
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);

    auto theme = provider->GetGroup("theme");
    ASSERT_TRUE(theme);
    theme->SetTransition(TransitionSpec{TimeSpan::Milliseconds(400), nullptr});

    // The alias already stands on something designed, so this is a change and moves over the duration.
    EXPECT_TRUE(theme->Define("v", *ConstructAny<float>(3.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);
    Update(TimeSpan::Milliseconds(0));
    Update(TimeSpan::Milliseconds(200));
    EXPECT_NEAR(c->GetValue(), 2.f, 0.01f);
    Update(TimeSpan::Milliseconds(200));
    EXPECT_FLOAT_EQ(c->GetValue(), 3.f);

    // Falling back to the default is a change too, and eases the same way.
    EXPECT_TRUE(provider->RemoveGroup("theme"));
    Update(TimeSpan::Milliseconds(0));
    Update(TimeSpan::Milliseconds(400));
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);
}

/**
 * @tc.name: DeclaredAliasIsListedAndTypedBeforeAnyValue
 * @tc.desc: A type hint makes an alias known to tooling before anything values it: it is listed and
 *           typed, but has no cell and no value. A value arriving later gives it one.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, DeclaredAliasIsListedAndTypedBeforeAnyValue, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    const auto floatType = ConstructAny<float>()->GetTypeId();
    EXPECT_FALSE(provider->SetAliasTypeHint("", floatType));
    EXPECT_FALSE(provider->SetAliasTypeHint("v", TypeId{}));
    ASSERT_TRUE(provider->SetAliasTypeHint("v", floatType, "theme"));

    // Declared, so tooling can offer it — but it holds nothing and nothing can read it yet.
    auto listed = provider->GetAliases();
    const auto* declared = Find(listed, "v");
    ASSERT_TRUE(declared);
    EXPECT_EQ(declared->type, floatType);
    // The catalog it was declared in, so tooling can present a project's aliases as they were authored.
    EXPECT_EQ(declared->group, "theme");
    EXPECT_FALSE(declared->hasCell);
    EXPECT_EQ(provider->GetAliasType("v"), floatType);
    EXPECT_FALSE(provider->GetAlias("v"));

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(1.f)));
    listed = provider->GetAliases();
    const auto* valued = Find(listed, "v");
    ASSERT_TRUE(valued);
    EXPECT_EQ(valued->type, floatType);
    EXPECT_TRUE(valued->hasCell);
    // One alias, however many ways it is known.
    EXPECT_EQ(listed.size(), 1u);
}

/**
 * @tc.name: HintCommitsNothingAndLosesToTheCell
 * @tc.desc: Declaring a type is not a promise: a value of another type is still accepted, and the type
 *           reported is the cell's, since that is the one consumers actually bound.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, HintCommitsNothingAndLosesToTheCell, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    ASSERT_TRUE(provider->SetAliasTypeHint("v", ConstructAny<float>()->GetTypeId()));

    const auto vecType = ConstructAny<BASE_NS::Math::Vec3>()->GetTypeId();
    EXPECT_TRUE(provider->Define("v", *ConstructAny<BASE_NS::Math::Vec3>({1.f, 2.f, 3.f})));
    EXPECT_EQ(provider->GetAliasType("v"), vecType);

    // Removing the cell leaves the declaration standing: it is metadata about the project, not a layer.
    EXPECT_TRUE(provider->Remove("v"));
    EXPECT_EQ(provider->GetAliasType("v"), ConstructAny<float>()->GetTypeId());

    // Withdrawing it is what a catalog re-applied without that entry does, and takes the alias with it.
    EXPECT_TRUE(provider->RemoveAliasTypeHint("v"));
    EXPECT_FALSE(provider->RemoveAliasTypeHint("v"));
    EXPECT_FALSE(provider->GetAliasType("v").IsValid());
    EXPECT_TRUE(provider->GetAliases().empty());
}

/**
 * @tc.name: ScopeSeesGloballyDeclaredAliases
 * @tc.desc: A scope inherits global aliases, so it lists and types the globally declared ones too.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, ScopeSeesGloballyDeclaredAliases, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto scope = MakeScope();
    const auto floatType = ConstructAny<float>()->GetTypeId();
    ASSERT_TRUE(provider->SetAliasTypeHint("global", floatType));
    ASSERT_TRUE(provider->SetAliasTypeHint("scoped", floatType, "", scope));

    auto aliases = provider->GetAliases(scope);
    EXPECT_TRUE(Contains(aliases, "scoped"));
    EXPECT_TRUE(Contains(aliases, "global"));
    EXPECT_EQ(provider->GetAliasType("global", scope), floatType);
    // The global scope sees only its own.
    EXPECT_FALSE(Contains(provider->GetAliases(), "scoped"));
}

/**
 * @tc.name: ListenerSeesTheAliasLifecycle
 * @tc.desc: A listener is told when an alias appears, when resolution decides it, when it is declared
 *           and when it goes — and may read the registry back while being told.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, ListenerSeesTheAliasLifecycle, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto listener = BASE_NS::shared_ptr<RecordingListener>(new RecordingListener);
    listener->registry = provider.get();
    provider->AddListener(interface_pointer_cast<IAliasListener>(listener));

    const auto floatType = ConstructAny<float>()->GetTypeId();
    EXPECT_TRUE(provider->SetAliasTypeHint("v", floatType, "theme"));
    EXPECT_EQ(listener->Count(AliasChangeType::DECLARED, "v"), 1u);
    // Read back from inside the callback: the declaration was in place before the listener was told.
    ASSERT_FALSE(listener->changes.empty());
    EXPECT_EQ(listener->changes.back().typeAtCall, floatType);

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(1.f)));
    EXPECT_EQ(listener->Count(AliasChangeType::ADDED, "v"), 1u);
    EXPECT_GE(listener->Count(AliasChangeType::RESOLVED, "v"), 1u);

    // Resolution ran even though the value did not move, because which layer decides it did.
    const auto resolvedBefore = listener->Count(AliasChangeType::RESOLVED, "v");
    auto theme = provider->GetGroup("theme");
    ASSERT_TRUE(theme);
    EXPECT_TRUE(theme->Define("v", *ConstructAny<float>(1.f)));
    EXPECT_GT(listener->Count(AliasChangeType::RESOLVED, "v"), resolvedBefore);

    EXPECT_TRUE(provider->Remove("v"));
    EXPECT_EQ(listener->Count(AliasChangeType::REMOVED, "v"), 1u);
    // The cell is gone by the time the listener hears, so only the declaration answers now.
    EXPECT_EQ(listener->changes.back().typeAtCall, floatType);

    EXPECT_TRUE(provider->RemoveAliasTypeHint("v"));
    EXPECT_EQ(listener->Count(AliasChangeType::DECLARED, "v"), 2u);
    EXPECT_FALSE(listener->changes.back().typeAtCall.IsValid());

    // Removed means removed: nothing after this reaches it.
    const auto seen = listener->changes.size();
    provider->RemoveListener(interface_pointer_cast<IAliasListener>(listener));
    EXPECT_TRUE(provider->Define("w", *ConstructAny<float>(2.f)));
    EXPECT_EQ(listener->changes.size(), seen);
}

/**
 * @tc.name: ListenerIsScopeAwareAndAddedOnce
 * @tc.desc: An event says which scope it belongs to, and adding the same listener twice does not make
 *           it hear everything twice.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, ListenerIsScopeAwareAndAddedOnce, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto scope = MakeScope();
    auto listener = BASE_NS::shared_ptr<RecordingListener>(new RecordingListener);
    auto asListener = interface_pointer_cast<IAliasListener>(listener);
    provider->AddListener(asListener);
    provider->AddListener(asListener);

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(1.f), scope));
    EXPECT_EQ(listener->Count(AliasChangeType::ADDED, "v"), 1u);
    ASSERT_FALSE(listener->changes.empty());
    EXPECT_TRUE(listener->changes.front().scoped);

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(2.f)));
    for (auto& c : listener->changes) {
        if (c.type == AliasChangeType::ADDED) {
            continue;
        }
        // Every remaining event belongs to one scope or the other, never to both.
        EXPECT_EQ(c.name, "v");
    }
    EXPECT_EQ(listener->Count(AliasChangeType::ADDED, "v"), 2u);
}

/**
 * @tc.name: ListenerIsHeldWeakly
 * @tc.desc: Listening does not keep the listener alive — a registry outlives what watches it — and a
 *           destroyed one is simply skipped rather than leaving the registry calling into nothing.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, ListenerIsHeldWeakly, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    BASE_NS::weak_ptr<IAliasListener> weak;
    {
        auto listener = BASE_NS::shared_ptr<RecordingListener>(new RecordingListener);
        auto asListener = interface_pointer_cast<IAliasListener>(listener);
        weak = asListener;
        provider->AddListener(asListener);
        EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(1.f)));
        EXPECT_EQ(listener->Count(AliasChangeType::ADDED, "v"), 1u);
    }
    // The registry held the only other reference it could have, and it holds none.
    EXPECT_TRUE(weak.expired());
    // Still usable afterwards: the dead slot is skipped, and dropped the next time one is added.
    EXPECT_TRUE(provider->Define("w", *ConstructAny<float>(2.f)));
    EXPECT_TRUE(provider->Remove("w"));
}

/**
 * @tc.name: GroupTransitionCascadesAndIsNotAContribution
 * @tc.desc: A group's transition covers the groups nested under it, and is a setting rather than a
 *           contribution: it neither makes the group exist nor goes away when its content is released.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, GroupTransitionCascadesAndIsNotAContribution, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto theme = provider->GetGroup("theme");
    ASSERT_TRUE(theme);
    theme->SetTransition(TransitionSpec{TimeSpan::Milliseconds(500), nullptr});

    // Stating it on a group nothing has been applied to is legal, and does not make the group exist.
    EXPECT_TRUE(provider->GetContributingGroups().empty());
    EXPECT_FALSE(provider->FindGroup("theme"));

    auto accent = provider->GetGroup("theme/accent");
    ASSERT_TRUE(accent);
    EXPECT_EQ(accent->GetTransition().duration, TimeSpan::Milliseconds(500));
    // A nested group that states its own is not overridden by the one above it.
    accent->SetTransition(TransitionSpec{TimeSpan::Milliseconds(100), nullptr});
    EXPECT_EQ(accent->GetTransition().duration, TimeSpan::Milliseconds(100));
    EXPECT_EQ(theme->GetTransition().duration, TimeSpan::Milliseconds(500));
    // Nothing on the path states one, so there is none.
    EXPECT_EQ(provider->GetGroup("other")->GetTransition().duration, TimeSpan::Zero());

    // Releasing the layer's content leaves how the layer behaves alone.
    EXPECT_TRUE(theme->Define("v", *ConstructAny<float>(1.f)));
    EXPECT_TRUE(provider->RemoveGroup("theme"));
    EXPECT_EQ(provider->GetGroup("theme")->GetTransition().duration, TimeSpan::Milliseconds(500));
}

/**
 * @tc.name: SettingsGroupsAreListedApartFromContributingOnes
 * @tc.desc: A setting neither makes a group exist nor goes away with its content, so the groups that
 *           state one are listed separately from the groups that contribute a value.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, SettingsGroupsAreListedApartFromContributingOnes, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();

    // A setting on a group nothing has been applied to: listed here, and nowhere else.
    provider->GetGroup("theme")->SetTransition(TransitionSpec{TimeSpan::Milliseconds(500), nullptr});
    EXPECT_TRUE(provider->GetContributingGroups().empty());
    ASSERT_EQ(provider->GetSettingsGroups().size(), 1u);
    EXPECT_EQ(provider->GetSettingsGroups().front(), "theme");

    // A value with no setting is the other way round.
    EXPECT_TRUE(provider->GetGroup("other")->Define("v", *ConstructAny<float>(1.f)));
    EXPECT_TRUE(Contains(provider->GetContributingGroups(), "other"));
    EXPECT_FALSE(Contains(provider->GetSettingsGroups(), "other"));

    // Shallowest first, as the layering reads.
    provider->GetGroup("theme/accent")->SetTransition(TransitionSpec{TimeSpan::Milliseconds(100), nullptr});
    auto settings = provider->GetSettingsGroups();
    ASSERT_EQ(settings.size(), 2u);
    EXPECT_EQ(settings[0], "theme");
    EXPECT_EQ(settings[1], "theme/accent");

    // Releasing a layer's content leaves what it states behind.
    EXPECT_TRUE(provider->GetGroup("theme")->Define("v", *ConstructAny<float>(2.f)));
    EXPECT_TRUE(Contains(provider->GetContributingGroups(), "theme"));
    EXPECT_TRUE(provider->RemoveGroup("theme"));
    EXPECT_FALSE(Contains(provider->GetContributingGroups(), "theme"));
    EXPECT_EQ(provider->GetSettingsGroups().size(), 2u);
}

/**
 * @tc.name: WithdrawnValueLeavesTheWayItCame
 * @tc.desc: A contribution carries the transition it arrived with, so removing the group it belongs to
 *           eases the alias back down rather than cutting it, even though the layer it falls back to
 *           states nothing.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, WithdrawnValueLeavesTheWayItCame, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v"));

    auto theme = provider->GetGroup("theme");
    ASSERT_TRUE(theme);
    EXPECT_TRUE(theme->Define("v", *ConstructAny<float>(1.f)));

    // The accent layer says how it comes and goes; the layer below it says nothing at all.
    auto accent = provider->GetGroup("theme/accent");
    ASSERT_TRUE(accent);
    accent->SetTransition(TransitionSpec{TimeSpan::Milliseconds(400), nullptr});
    EXPECT_TRUE(accent->Define("v", *ConstructAny<float>(3.f)));
    Update(TimeSpan::Milliseconds(0));
    Update(TimeSpan::Milliseconds(400));
    EXPECT_FLOAT_EQ(c->GetValue(), 3.f);

    // Taking it away is a change like any other, and uses the transition the contribution carried.
    EXPECT_TRUE(provider->RemoveGroup("theme/accent"));
    EXPECT_FLOAT_EQ(c->GetValue(), 3.f);
    Update(TimeSpan::Milliseconds(0));
    Update(TimeSpan::Milliseconds(200));
    EXPECT_NEAR(c->GetValue(), 2.f, 0.01f);
    Update(TimeSpan::Milliseconds(200));
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);
}

/**
 * @tc.name: UninterpolatableValueChangesAtOnce
 * @tc.desc: A transition on a type with nothing meaningful between two values changes at once rather
 *           than holding the old value for the duration and swapping at the end.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, UninterpolatableValueChangesAtOnce, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto c = ConstructProperty<BASE_NS::string>("c", "");
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v"));

    auto theme = provider->GetGroup("theme");
    ASSERT_TRUE(theme);
    theme->SetTransition(TransitionSpec{TimeSpan::Milliseconds(500), nullptr});
    EXPECT_TRUE(theme->Define("v", *ConstructAny<BASE_NS::string>("first")));
    EXPECT_TRUE(theme->Define("v", *ConstructAny<BASE_NS::string>("second")));
    EXPECT_EQ(c->GetValue(), "second");
}

/**
 * @tc.name: ScopeInheritsTheGlobalAlias
 * @tc.desc: An alias in a scope that supplies nothing for it reads through to the global one, and keeps
 *           reading through as the global value changes — a scope layers on the registry rather than
 *           partitioning it.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, ScopeInheritsTheGlobalAlias, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto scope = MakeScope();
    ASSERT_TRUE(scope);

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(1.f)));

    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v", scope));
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);

    // Live: the scope has no value of its own, so it follows the global one wherever it goes.
    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(5.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 5.f);
}

/**
 * @tc.name: ScopedMaskBlocksInheritance
 * @tc.desc: A mask states that the alias has no value in this scope, so the walk stops there rather than
 *           reading through to the global alias, and the alias falls to its own default. Withdrawing the
 *           mask puts the scope back to inheriting.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, ScopedMaskBlocksInheritance, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto scope = MakeScope();
    ASSERT_TRUE(scope);

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(1.f)));
    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v", scope));
    ASSERT_TRUE(provider->SetDefault("v", ConstructAny<float>(9.f), scope));
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);

    // Masked in the scope: the global value is not inherited, so the alias sits on its own default.
    auto accent = provider->GetGroup("theme/accent", scope);
    ASSERT_TRUE(accent);
    EXPECT_TRUE(accent->Mask("v"));
    EXPECT_FLOAT_EQ(c->GetValue(), 9.f);

    // The global alias goes on changing underneath without reaching the scope that masked it.
    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(2.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 9.f);

    // Withdrawing the mask puts the scope back to inheriting.
    EXPECT_TRUE(accent->Undefine("v"));
    EXPECT_FLOAT_EQ(c->GetValue(), 2.f);
}

/**
 * @tc.name: ScopedMaskIsBeatenByADeeperScopedValue
 * @tc.desc: A mask is a layer like any other, so a deeper layer supplying a value still wins it — the
 *           mask only ends the walk when it is the layer that won.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, ScopedMaskIsBeatenByADeeperScopedValue, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto scope = MakeScope();

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(1.f)));
    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v", scope));

    EXPECT_TRUE(provider->GetGroup("theme", scope)->Mask("v"));
    EXPECT_FLOAT_EQ(c->GetValue(), 0.f);

    EXPECT_TRUE(provider->GetGroup("theme/accent", scope)->Define("v", *ConstructAny<float>(3.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 3.f);

    // And an ungrouped Define in the scope beats both, as it does anywhere.
    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(4.f), scope));
    EXPECT_FLOAT_EQ(c->GetValue(), 4.f);
}

/**
 * @tc.name: RemovingTheGlobalAliasReachesTheScopesInheritingIt
 * @tc.desc: Removing an alias destroys its cell rather than writing it, so nothing is notified — a scope
 *           that was reading through to it must still be told, or it keeps the value it last inherited.
 *           A scope supplying its own value is unaffected, as it was never inheriting.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, RemovingTheGlobalAliasReachesTheScopesInheritingIt, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto inheriting = MakeScope();
    auto deciding = MakeScope();

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(1.f)));
    auto a = ConstructProperty<float>("a", 7.f);
    auto b = ConstructProperty<float>("b", 7.f);
    ASSERT_TRUE(provider->Bind(a.GetProperty(), "v", inheriting));
    ASSERT_TRUE(provider->Bind(b.GetProperty(), "v", deciding));
    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(2.f), deciding));
    EXPECT_FLOAT_EQ(a->GetValue(), 1.f);
    EXPECT_FLOAT_EQ(b->GetValue(), 2.f);

    EXPECT_TRUE(provider->Remove("v"));
    // The scope's own cell survives — only what it was inheriting is gone — so its consumer falls back
    // to the cell's default rather than to its own value.
    EXPECT_FLOAT_EQ(a->GetValue(), 0.f);
    EXPECT_FLOAT_EQ(b->GetValue(), 2.f);
}

/**
 * @tc.name: ScopeOverridesOneAliasOnly
 * @tc.desc: A scope defining an alias decides it for itself and for nothing else — the global alias and
 *           every other scope are untouched, which is what substituting a whole registry could not do.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, ScopeOverridesOneAliasOnly, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto one = MakeScope();
    auto other = MakeScope();

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(1.f)));
    auto a = ConstructProperty<float>("a", 0.f);
    auto b = ConstructProperty<float>("b", 0.f);
    auto g = ConstructProperty<float>("g", 0.f);
    ASSERT_TRUE(provider->Bind(a.GetProperty(), "v", one));
    ASSERT_TRUE(provider->Bind(b.GetProperty(), "v", other));
    ASSERT_TRUE(provider->Bind(g.GetProperty(), "v"));

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(2.f), one));
    EXPECT_FLOAT_EQ(a->GetValue(), 2.f);
    EXPECT_FLOAT_EQ(b->GetValue(), 1.f);
    EXPECT_FLOAT_EQ(g->GetValue(), 1.f);

    // Withdrawing it puts the scope back to inheriting, live.
    EXPECT_TRUE(provider->Undefine("v", one));
    EXPECT_FLOAT_EQ(a->GetValue(), 1.f);
}

/**
 * @tc.name: ScopedGroupsLayerAboveTheGlobalAlias
 * @tc.desc: Groups are scoped too, and the whole stack sits above what the scope inherits: a scoped
 *           group beats the global alias, and a scoped ungrouped Define beats the scoped group.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, ScopedGroupsLayerAboveTheGlobalAlias, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto scope = MakeScope();

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(1.f)));
    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v", scope));
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);

    auto theme = provider->GetGroup("theme", scope);
    ASSERT_TRUE(theme);
    EXPECT_TRUE(theme->Define("v", *ConstructAny<float>(3.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 3.f);

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(4.f), scope));
    EXPECT_FLOAT_EQ(c->GetValue(), 4.f);

    // The same path in another scope is a different layer, so removing it there changes nothing here.
    EXPECT_FALSE(provider->RemoveGroup("theme", MakeScope()));
    EXPECT_FLOAT_EQ(c->GetValue(), 4.f);

    // Dropping the scope's own layers walks back down to what it inherits.
    EXPECT_TRUE(provider->Undefine("v", scope));
    EXPECT_TRUE(provider->RemoveGroup("theme", scope));
    EXPECT_FLOAT_EQ(c->GetValue(), 1.f);
}

/**
 * @tc.name: InheritanceConvertsIntoTheScopeType
 * @tc.desc: The global alias and a scope's may be typed by different first references — a document's bare
 *           number globally, a float consumer here — so an inherited value is converted into the type the
 *           scope's own cell holds rather than refused.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, InheritanceConvertsIntoTheScopeType, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto scope = MakeScope();

    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v", scope));

    // Nothing bound globally, so the global cell takes its type from the value it is given.
    EXPECT_TRUE(provider->Define("v", LooseDouble{0.8}));
    EXPECT_FLOAT_EQ(c->GetValue(), 0.8f);
}

/**
 * @tc.name: IncompatibleGlobalAliasIsNotInherited
 * @tc.desc: A scope's alias is typed by its own first reference, so a global namesake of another type is
 *           not inherited — the scope keeps its own default rather than the bind failing.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, IncompatibleGlobalAliasIsNotInherited, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto scope = MakeScope();

    EXPECT_TRUE(provider->Define("v", *ConstructAny<BASE_NS::string>("global")));

    auto c = ConstructProperty<float>("c", 7.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v", scope));
    EXPECT_FLOAT_EQ(c->GetValue(), 0.f);

    // It still works as an alias in its own right.
    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(2.f), scope));
    EXPECT_FLOAT_EQ(c->GetValue(), 2.f);
}

/**
 * @tc.name: FirstGlobalDefineReachesAnAlreadyBoundScope
 * @tc.desc: A scope that bound an alias before anything defined it still picks the global value up when
 *           one first appears — inheritance does not depend on the global alias existing first.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AliasRegistryTest, FirstGlobalDefineReachesAnAlreadyBoundScope, testing::ext::TestSize.Level1)
{
    auto provider = MakeRegistry();
    auto scope = MakeScope();

    auto c = ConstructProperty<float>("c", 0.f);
    ASSERT_TRUE(provider->Bind(c.GetProperty(), "v", scope));
    EXPECT_FLOAT_EQ(c->GetValue(), 0.f);

    EXPECT_TRUE(provider->Define("v", *ConstructAny<float>(3.f)));
    EXPECT_FLOAT_EQ(c->GetValue(), 3.f);
}

}  // namespace UTest
META_END_NAMESPACE()