/**
 *  Copyright (C) 2022 Huawei Technologies Co, Ltd.
 */
#include <tester.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/api/object.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_proxy_object.h>

#include "util.h"

META_BEGIN_NAMESPACE()
namespace Test {

using namespace testing;

class ProxyObjectTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        object_.AddProperty(ConstructProperty<float>(GetObjectRegistry(), Prop1, 42.f));
        object_.AddProperty(ConstructProperty<float>(GetObjectRegistry(), Prop2, 21.f));

        proxy_ = GetObjectRegistry().Create<IProxyObject>(ClassId::ProxyObject);
        ASSERT_TRUE(proxy_);
        ASSERT_TRUE(proxy_->SetTarget(object_));
    }
    void TearDown() override {}

protected:
    static constexpr auto Prop1 = "prop1";
    static constexpr auto Prop2 = "prop2";

    void SetPropValue(const char* propertyName, float value)
    {
        object_.Metadata().GetProperty<float>(propertyName)->SetValue(value);
    }

    Object object_;
    IProxyObject::Ptr proxy_;
};

TEST_F(ProxyObjectTest, Proxy)
{
    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    auto meta = interface_pointer_cast<IMetadata>(proxy_);
    ASSERT_TRUE(meta);

    auto prop1 = meta->GetProperty<float>(Prop1);
    auto prop2 = meta->GetProperty<float>(Prop2);

    ASSERT_TRUE(prop1);
    ASSERT_TRUE(prop2);

    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    SetPropValue(Prop1, 62.f);
    // Proxy should follow source object properties
    EXPECT_EQ(prop1->GetValue(), 62.f);

    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    // Break bind
    prop1->SetValue(2.f);
    SetPropValue(Prop1, 82.f);
    EXPECT_EQ(prop1->GetValue(), 2.f);

    ASSERT_TRUE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);
}

TEST_F(ProxyObjectTest, PropertyInProxy)
{
    auto meta = interface_pointer_cast<IMetadata>(proxy_);
    ASSERT_TRUE(meta);

    auto p = CreateProperty<int>("test");
    meta->AddProperty(p);

    auto prop1 = meta->GetProperty<float>(Prop1);
    auto prop2 = meta->GetProperty<float>(Prop2);

    ASSERT_TRUE(prop1);
    ASSERT_TRUE(prop2);

    EXPECT_FALSE(proxy_->GetProxyProperty("test"));
    EXPECT_TRUE(proxy_->GetProxyProperty(Prop1));

    ASSERT_TRUE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    meta->RemoveProperty(p);

    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);
}

TEST_F(ProxyObjectTest, AddProperty)
{
    static constexpr auto prop3Name = "prop3";

    // Add a property to object, it should automatically be added to proxy
    auto prop3 = ConstructProperty<float>(GetObjectRegistry(), prop3Name, 2.f);
    object_.AddProperty(prop3);

    auto meta = interface_pointer_cast<IMetadata>(proxy_);
    ASSERT_TRUE(meta);

    auto prop3Proxy = meta->GetProperty<float>(prop3Name);
    ASSERT_TRUE(prop3Proxy);

    prop3->SetValue(42.f);
    EXPECT_EQ(prop3Proxy->GetValue(), 42.f);
}

TEST_F(ProxyObjectTest, RemoveProperty)
{
    // Remove prop2 from our source object
    auto prop2 = object_.Metadata().GetProperty<float>(Prop2);
    ASSERT_TRUE(prop2);
    object_.RemoveProperty(prop2);
    ASSERT_FALSE(object_.Metadata().GetProperty<float>(Prop2));

    // Property should be removed from proxy object as well
    auto meta = interface_pointer_cast<IMetadata>(proxy_);
    ASSERT_TRUE(meta);
    ASSERT_FALSE(meta->GetProperty<float>(Prop2));

    auto prop1Proxy = meta->GetProperty<float>(Prop1);
    ASSERT_TRUE(prop1Proxy);
    prop1Proxy->SetValue(100.f); // Break default bind

    // Remove property from object_, it should not be removed from proxy since
    // it has been set to a specific value
    auto prop1 = object_.Metadata().GetProperty<float>(Prop1);
    ASSERT_TRUE(prop1);
    object_.RemoveProperty(prop2);
    prop1Proxy = meta->GetProperty<float>(Prop1);
    ASSERT_TRUE(prop1Proxy);
    EXPECT_EQ(prop1Proxy->GetValue(), 100.f);
}

TEST_F(ProxyObjectTest, Dynamic)
{
    proxy_->Dynamic()->SetValue(false);
    ASSERT_TRUE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);
    proxy_->Dynamic()->ResetValue();
    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    static constexpr auto prop3Name = "prop3";
    static constexpr auto prop4Name = "prop4";

    // Add a property to object, it should not be added to proxy since Dynamic=false
    auto prop3 = ConstructProperty<float>(GetObjectRegistry(), prop3Name, 2.f);
    object_.AddProperty(prop3);

    auto meta = interface_pointer_cast<META_NS::IMetadata>(proxy_);
    ASSERT_TRUE(meta);

    EXPECT_TRUE(meta->GetProperty<float>(prop3Name));

    // Set Dynamic to false, then remove one property (it should be gone from proxy metadata).
    // Then, set dynamic back to true, the property should re-appear
    proxy_->Dynamic()->SetValue(false);
    object_.RemoveProperty(prop3);
    EXPECT_TRUE(meta->GetProperty<float>(prop3Name));
    proxy_->Dynamic()->SetValue(true);
    EXPECT_FALSE(meta->GetProperty<float>(prop3Name));
}

TEST_F(ProxyObjectTest, ProxyOfProxy)
{
    // Create a secondary proxy object which proxies the proxy
    // So we have object_ <- proxy_ <- proxy2
    auto proxy2 = GetObjectRegistry().Create<IProxyObject>(ClassId::ProxyObject);
    ASSERT_TRUE(proxy2);
    ASSERT_TRUE(proxy2->SetTarget(interface_pointer_cast<IObject>(proxy_)));

    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy2)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    static constexpr auto prop3Name = "prop3";
    // Add a property to object, it should be added to proxy of proxy
    auto prop3 = ConstructProperty<float>(GetObjectRegistry(), prop3Name, 2.f);
    object_.AddProperty(prop3);

    auto meta = interface_pointer_cast<IMetadata>(proxy2);
    ASSERT_TRUE(meta);
    auto prop3Proxy2 = meta->GetProperty<float>(prop3Name);
    ASSERT_TRUE(prop3Proxy2);

    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy2)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    prop3->SetValue(42.f);
    EXPECT_EQ(prop3Proxy2->GetValue(), 42.f);

    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy2)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    // Remove prop3 from object_, should be gone from proxy2 as well
    object_.RemoveProperty(prop3);
    EXPECT_FALSE(meta->GetProperty<float>(prop3Name));

    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy2)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    auto p1meta = interface_pointer_cast<IMetadata>(proxy_);
    ASSERT_TRUE(p1meta);

    auto prop1 = p1meta->GetProperty<float>(Prop1);
    prop1->SetValue(55);

    auto prop1Proxy2 = meta->GetProperty<float>(Prop1);
    ASSERT_TRUE(prop1Proxy2);
    EXPECT_EQ(prop1Proxy2->GetValue(), 55);
}

TEST_F(ProxyObjectTest, ProxyMember)
{
    Object p2obj;
    p2obj.AddProperty(CreateProperty<int>("pint", 2));
    auto proxy2 = GetObjectRegistry().Create<IProxyObject>(ClassId::ProxyObject);
    proxy2->SetTarget(p2obj);
    interface_cast<IMetadata>(proxy_)->AddProperty(
        CreateProperty<IObject::Ptr>("proxy", interface_pointer_cast<IObject>(proxy2)));

    auto p1m = interface_cast<IMetadata>(proxy_);
    ASSERT_TRUE(p1m);
    {
        auto p = p1m->GetProperty<IObject::Ptr>("proxy");
        auto p2m = interface_cast<IMetadata>(p->GetValue());
        ASSERT_TRUE(p2m);
        auto pint = p2m->GetProperty<int>("pint");
        ASSERT_TRUE(pint);
        EXPECT_EQ(pint->GetValue(), 2);
    }

    // Create new proxy hierarchy and set it as target
    Object object;
    auto nproxy = GetObjectRegistry().Create<IProxyObject>(ClassId::ProxyObject);
    ASSERT_TRUE(nproxy);

    Object np2obj;
    np2obj.AddProperty(CreateProperty<int>("pint", 7));
    np2obj.AddProperty(CreateProperty<int>("pint2", 2));
    auto nproxy2 = GetObjectRegistry().Create<IProxyObject>(ClassId::ProxyObject);
    nproxy2->SetTarget(np2obj);
    interface_cast<IMetadata>(nproxy)->AddProperty(
        CreateProperty<IObject::Ptr>("proxy", interface_pointer_cast<IObject>(nproxy2)));

    ASSERT_TRUE(nproxy->SetTarget(object));
    proxy_->SetTarget(interface_pointer_cast<IObject>(nproxy));

    {
        auto p = p1m->GetProperty<IObject::Ptr>("proxy");
        auto p2m = interface_cast<IMetadata>(p->GetValue());
        ASSERT_TRUE(p2m);
        auto pint = p2m->GetProperty<int>("pint");
        ASSERT_TRUE(pint);
        EXPECT_EQ(pint->GetValue(), 7);

        auto pint2 = p2m->GetProperty<int>("pint2");
        ASSERT_TRUE(pint2);
        EXPECT_EQ(pint2->GetValue(), 2);
    }
}

TEST_F(ProxyObjectTest, SetPropertyTarget)
{
    auto meta = interface_pointer_cast<IMetadata>(proxy_);
    ASSERT_TRUE(meta);

    auto prop1 = meta->GetProperty<float>(Prop1);
    ASSERT_TRUE(prop1);

    auto ep = ConstructProperty<float>(GetObjectRegistry(), Prop1, 99.f);

    EXPECT_EQ(prop1->GetValue(), 42.f);
    EXPECT_FALSE(proxy_->GetOverride(Prop1));

    EXPECT_EQ(proxy_->SetPropertyTarget(ep), prop1.GetProperty());
    EXPECT_EQ(prop1->GetValue(), 99.f);
    EXPECT_FALSE(proxy_->GetOverride(Prop1));

    prop1->SetValue(2.f);
    EXPECT_EQ(prop1->GetValue(), 2.f);
    EXPECT_TRUE(proxy_->GetOverride(Prop1));

    prop1->ResetValue();
    EXPECT_EQ(prop1->GetValue(), 99.f);

    meta->RemoveProperty(prop1);

    prop1 = meta->GetProperty<float>(Prop1);
    ASSERT_TRUE(prop1);
    EXPECT_EQ(prop1->GetValue(), 42.f);
    EXPECT_FALSE(proxy_->GetOverride(Prop1));
}

TEST_F(ProxyObjectTest, InternalBindingIsScheduledForCreatingBeforeGettinProxyProperty)
{
    proxy_->AddInternalProxy(Prop2, Prop1);

    auto objectProp1 = object_.Metadata().GetProperty<float>(Prop1);
    auto objectProp2 = object_.Metadata().GetProperty<float>(Prop2);
    auto proxyProperty = interface_cast<IMetadata>(proxy_)->GetProperty<float>(Prop1);

    auto internalProxyProp2 = interface_cast<IMetadata>(proxy_)->GetProperty<float>(Prop2);

    objectProp1->SetValue(2000.0F);
    objectProp2->SetValue(3000.0F);

    // proxy::prop2 is internal bind to proxy::prop1, proxy::prop1 is bind to object::prop1,
    // so setting object::prop2 should not be considered in proxy
    EXPECT_THAT(proxyProperty->GetValue(), FloatEq(2000.0F));
    EXPECT_THAT(internalProxyProp2->GetValue(), FloatEq(2000.0F));

    proxyProperty->SetValue(1000.0F);
    // proxy::prop2 is internal bind to proxy::prop1, so we should have changed value
    EXPECT_THAT(internalProxyProp2->GetValue(), FloatEq(1000.0F));
}

TEST_F(ProxyObjectTest, InternalBindingRemovesBindToTargetAndBindToInternalProperty)
{
    auto objectProp1 = object_.Metadata().GetProperty<float>(Prop1);
    auto objectProp2 = object_.Metadata().GetProperty<float>(Prop2);
    auto proxyProperty = interface_cast<IMetadata>(proxy_)->GetProperty<float>(Prop1);

    auto internalProxyProp2 = interface_cast<IMetadata>(proxy_)->GetProperty<float>(Prop2);

    proxy_->AddInternalProxy(Prop2, Prop1);

    objectProp1->SetValue(2000.0F);
    objectProp2->SetValue(3000.0F);
    // proxy::prop2 is internal bind to proxy::prop1, proxy::prop1 is bind to object::prop1,
    // so setting object::prop2 should not be considered in proxy
    EXPECT_THAT(internalProxyProp2->GetValue(), FloatEq(2000.0F));

    proxyProperty->SetValue(1000.0F);
    // proxy::prop2 is internal bind to proxy::prop1, so we should have changed value
    EXPECT_THAT(internalProxyProp2->GetValue(), FloatEq(1000.0F));
}

TEST_F(ProxyObjectTest, InternalBindWorksAfterChangingProxyTarget)
{
    proxy_->AddInternalProxy(Prop2, Prop1);

    auto proxyProperty = interface_cast<IMetadata>(proxy_)->GetProperty<float>(Prop1);
    auto internalProxyProp2 = interface_cast<IMetadata>(proxy_)->GetProperty<float>(Prop2);

    auto anotherObject = Object();
    anotherObject.AddProperty(ConstructProperty<float>(GetObjectRegistry(), Prop1, 100.F));
    anotherObject.AddProperty(ConstructProperty<float>(GetObjectRegistry(), Prop2, 200.F));

    proxy_->SetTarget(anotherObject);

    EXPECT_THAT(internalProxyProp2->GetValue(), FloatEq(100.0F));

    proxyProperty->SetValue(300.0F);

    EXPECT_THAT(internalProxyProp2->GetValue(), FloatEq(300.0F));
}

TEST_F(ProxyObjectTest, InternalBindIsRemovedWhenNewTargetDoesNotHaveSourceProperty)
{
    proxy_->AddInternalProxy(Prop2, Prop1);

    auto internalProxyProp2 = interface_cast<IMetadata>(proxy_)->GetProperty<float>(Prop2);

    auto anotherObject = Object();
    anotherObject.AddProperty(ConstructProperty<float>(GetObjectRegistry(), Prop2, 200.F));

    auto objectProp2 = anotherObject.Metadata().GetProperty<float>(Prop2);

    proxy_->SetTarget(anotherObject);
    EXPECT_THAT(internalProxyProp2->GetValue(), FloatEq(200.0F));

    objectProp2->SetValue(800.0F);
    EXPECT_THAT(internalProxyProp2->GetValue(), FloatEq(800.0F));
}

} // namespace Test
META_END_NAMESPACE()
