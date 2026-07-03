/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <meta/api/object.h>
#include <meta/ext/any_builder.h>
#include <meta/interface/detail/enum.h>
#include <meta/interface/enum_macros.h>
#include <meta/interface/intf_object_registry.h>

#include "helpers/serialisation_utils.h"
#include "helpers/testing_objects.h"
#include "helpers/util.h"

META_BEGIN_NAMESPACE()
enum class MyEnum {
    MyA = 1,
    MyB = 2,
    MyC = 4,
};

META_BEGIN_ENUM(MyEnum, "Nice Enum", MyEnum::MyA)
META_ENUM_VALUE(MyEnum::MyA, "MyA", "My A enumerator")
META_ENUM_VALUE(MyEnum::MyB, "MyB", "My B enumerator")
META_ENUM_VALUE(MyEnum::MyC, "MyC", "My C enumerator")
META_END_ENUM()

META_ENUM_TYPE(MyEnum)

enum class MyBitFieldEnum : uint32_t {
    MyA = 1,
    MyB = 2,
    MyC = 4,
};

META_BEGIN_BITFIELD_ENUM(MyBitFieldEnum, "Nice Enum", MyBitFieldEnum(0))
META_ENUM_VALUE(MyBitFieldEnum::MyA, "MyA", "My A enumerator")
META_ENUM_VALUE(MyBitFieldEnum::MyB, "MyB", "My B enumerator")
META_ENUM_VALUE(MyBitFieldEnum::MyC, "MyC", "My C enumerator")
META_END_ENUM()

META_ENUM_TYPE(MyBitFieldEnum)

enum class PlainEnum : uint32_t { Hops = 1 };

META_TYPE(PlainEnum)

enum class SignedEnum : int32_t { Neg = -5, Pos = 7 };

META_TYPE(SignedEnum)

enum class SignedByteEnum : int8_t { Neg = -1, Pos = 100 };

META_TYPE(SignedByteEnum)

enum class Unsigned64Enum : uint64_t { Lo = 1, Hi = 0x8000000000000000ULL };

META_TYPE(Unsigned64Enum)

namespace UTest {

/**
 * @tc.name: SingleValue
 * @tc.desc: Tests for Single Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, SingleValue, testing::ext::TestSize.Level1)
{
    Enum<MyEnumMetadata> v;

    EXPECT_EQ(v.GetName(), "MyEnum");
    EXPECT_EQ(v.GetDescription(), "Nice Enum");
    EXPECT_EQ(v.GetEnumType(), EnumType::SINGLE_VALUE);
    auto members = v.GetValues();
    ASSERT_EQ(members.size(), 3);
    EXPECT_EQ(members[0].name, "MyA");
    EXPECT_EQ(members[0].desc, "My A enumerator");
    EXPECT_EQ(members[1].name, "MyB");
    EXPECT_EQ(members[1].desc, "My B enumerator");
    EXPECT_EQ(members[2].name, "MyC");
    EXPECT_EQ(members[2].desc, "My C enumerator");
    EXPECT_EQ(v.GetValueIndex(), 0);
    EXPECT_TRUE(v.IsValueSet(0));
    EXPECT_FALSE(v.IsValueSet(1));
    EXPECT_FALSE(v.IsValueSet(6));
    EXPECT_TRUE(v.SetValueIndex(1));
    EXPECT_EQ(v.GetValueIndex(), 1);
    EXPECT_TRUE(v.IsValueSet(1));
    EXPECT_TRUE(v.SetValueIndex(2));
    EXPECT_EQ(v.GetValueIndex(), 2);
    EXPECT_TRUE(v.IsValueSet(2));
    EXPECT_FALSE(v.SetValueIndex(3));
}

/**
 * @tc.name: BitField
 * @tc.desc: Tests for Bit Field. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, BitField, testing::ext::TestSize.Level1)
{
    Enum<MyBitFieldEnumMetadata> v;

    EXPECT_EQ(v.GetValueIndex(), -1);
    EXPECT_FALSE(v.IsValueSet(0));
    EXPECT_FALSE(v.IsValueSet(1));
    EXPECT_FALSE(v.IsValueSet(6));
    EXPECT_TRUE(v.SetValueIndex(1));
    EXPECT_EQ(v.GetValueIndex(), 1);
    EXPECT_TRUE(v.IsValueSet(1));
    EXPECT_TRUE(v.SetValueIndex(2));
    EXPECT_EQ(v.GetValueIndex(), 2);
    EXPECT_TRUE(v.IsValueSet(2));
    EXPECT_FALSE(v.SetValueIndex(3));

    EXPECT_TRUE(v.FlipValue(2, false));
    EXPECT_EQ(v.GetValueIndex(), -1);

    EXPECT_TRUE(v.FlipValue(0, true));
    EXPECT_EQ(v.GetValueIndex(), 0);
    EXPECT_TRUE(v.FlipValue(1, true));
    EXPECT_EQ(v.GetValueIndex(), -1);
    EXPECT_TRUE(v.IsValueSet(0));
    EXPECT_TRUE(v.IsValueSet(1));
    EXPECT_FALSE(v.IsValueSet(2));
}

/**
 * @tc.name: Compatibility
 * @tc.desc: Tests for Compatibility. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, Compatibility, testing::ext::TestSize.Level1)
{
    Enum<MyEnumMetadata> v1;
    EXPECT_TRUE(IsCompatible(v1, UidFromType<MyEnum>()));
    EXPECT_TRUE(IsCompatible(v1, UidFromType<int64_t>()));
    Enum<MyBitFieldEnumMetadata> v2;
    EXPECT_TRUE(IsCompatible(v2, UidFromType<MyBitFieldEnum>()));
    EXPECT_TRUE(IsCompatible(v2, UidFromType<uint64_t>()));
}

/**
 * @tc.name: GetSetData
 * @tc.desc: Tests for Get Set Data. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, GetSetData, testing::ext::TestSize.Level1)
{
    {
        Enum<MyEnumMetadata> v;
        EXPECT_TRUE(v.SetValue(MyEnum::MyB));
        EXPECT_EQ(GetValue<MyEnum>(v), MyEnum::MyB);
        EXPECT_EQ(GetValue<int64_t>(v), 2);
        EXPECT_TRUE(v.SetValue<int64_t>(4));
        EXPECT_EQ(GetValue<MyEnum>(v), MyEnum::MyC);
        EXPECT_FALSE(v.SetValue<int64_t>(5));
    }
    {
        Enum<MyBitFieldEnumMetadata> v;
        EXPECT_TRUE(v.SetValue(MyBitFieldEnum::MyB));
        EXPECT_EQ(GetValue<MyBitFieldEnum>(v), MyBitFieldEnum::MyB);
        EXPECT_EQ(GetValue<uint64_t>(v), 2);
        EXPECT_TRUE(v.SetValue<uint64_t>(4));
        EXPECT_EQ(GetValue<MyBitFieldEnum>(v), MyBitFieldEnum::MyC);
        EXPECT_TRUE(v.SetValue<uint64_t>(5));
        EXPECT_EQ(GetValue<uint64_t>(v), 5);
    }
}

/**
 * @tc.name: CopyFrom
 * @tc.desc: Tests for Copy From. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, CopyFrom, testing::ext::TestSize.Level1)
{
    Enum<MyEnumMetadata> v;
    auto p = v.Clone();
    EXPECT_EQ(GetValue<int64_t>(*p), 1);
    EXPECT_TRUE(v.SetValue(MyEnum::MyB));
    EXPECT_TRUE(p->CopyFrom(v));
    EXPECT_EQ(GetValue<int64_t>(*p), 2);

    Any<int64_t> any{4};
    EXPECT_TRUE(p->CopyFrom(any));
    EXPECT_EQ(GetValue<int64_t>(*p), 4);
    EXPECT_TRUE(any.SetValue<int64_t>(5));
    EXPECT_FALSE(p->CopyFrom(any));
    EXPECT_EQ(GetValue<int64_t>(*p), 4);

    EXPECT_TRUE(any.CopyFrom(*p));
    EXPECT_EQ(GetValue<int64_t>(any), 4);
}

/**
 * @tc.name: ArrayEnum
 * @tc.desc: Tests for Array Enum. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, ArrayEnum, testing::ext::TestSize.Level1)
{
    ArrayEnum<MyEnumMetadata> arr{MyEnum::MyA, MyEnum::MyB, MyEnum::MyC};

    auto any = arr.Clone({CloneValueType::COPY_VALUE, TypeIdRole::ITEM});
    ASSERT_TRUE(any);
    EXPECT_TRUE(arr.GetAnyAt(0, *any));
    EXPECT_EQ(GetValue<int64_t>(*any), 1);
    EXPECT_EQ(GetValue<MyEnum>(*any), MyEnum::MyA);
}

/**
 * @tc.name: Serialise
 * @tc.desc: Tests for Serialise. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, Serialise, testing::ext::TestSize.Level1)
{
    RegisterUserAny<Enum<MyEnumMetadata>>();
    TestSerialiser ser;
    {
        Object obj(CreateInstance(ClassId::Object));
        Metadata(obj).AddProperty(ConstructProperty<MyEnum>("test"));
        ASSERT_TRUE(ser.Export(obj));
    }
    ser.Dump("app://enum.json");

    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    auto p = obj->GetProperty<MyEnum>("test");
    ASSERT_TRUE(p);

    EXPECT_EQ(p->GetValue(), MyEnum::MyA);

    UnregisterUserAny<Enum<MyEnumMetadata>>();
}

/**
 * @tc.name: SerialiseBitField
 * @tc.desc: Tests for Serialise Bit Field. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, SerialiseBitField, testing::ext::TestSize.Level1)
{
    RegisterUserAny<Enum<MyBitFieldEnumMetadata>>();
    TestSerialiser ser;
    {
        Object obj(CreateInstance(ClassId::Object));
        auto p = ConstructProperty<MyBitFieldEnum>("test");
        auto en = interface_pointer_cast<IEnum>(p->GetValueAny().Clone());
        ASSERT_TRUE(en);
        en->FlipValue(0, true);
        en->FlipValue(2, true);
        p->SetValueAny(*interface_cast<IAny>(en));
        Metadata(obj).AddProperty(p);
        ASSERT_TRUE(ser.Export(obj));
    }
    ser.Dump("app://enum_bitfield.json");

    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    auto p = obj->GetProperty<MyBitFieldEnum>("test");
    ASSERT_TRUE(p);

    EXPECT_EQ(p->GetValue(), MyBitFieldEnum(5));

    UnregisterUserAny<Enum<MyBitFieldEnumMetadata>>();
}

/**
 * @tc.name: PlainEnum
 * @tc.desc: Tests for Plain Enum. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, PlainEnum, testing::ext::TestSize.Level1)
{
    auto p = ConstructProperty<PlainEnum>("test");
    ASSERT_TRUE(p);
    Property<uint64_t> prop{p.GetProperty()};
    ASSERT_TRUE(prop);
    prop->SetValue(1);
    EXPECT_EQ(prop->GetValue(), 1);
}

/**
 * @tc.name: SignedNegative
 * @tc.desc: Negative values of a signed enum round-trip through the erased enum any (int64 compat read must
 *           be sign-extended; the typed read truncates back). Covers 4-byte and 1-byte underlying widths.
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, SignedNegative, testing::ext::TestSize.Level1)
{
    {
        auto p = ConstructProperty<SignedEnum>("test");
        ASSERT_TRUE(p);
        Property<int64_t> prop{p.GetProperty()};
        ASSERT_TRUE(prop);

        prop->SetValue(-5);
        EXPECT_EQ(prop->GetValue(), -5);            // int64 compat read is sign-extended
        EXPECT_EQ(p->GetValue(), SignedEnum::Neg);  // typed read truncates back

        prop->SetValue(7);
        EXPECT_EQ(prop->GetValue(), 7);
        EXPECT_EQ(p->GetValue(), SignedEnum::Pos);
    }
    {
        // Smallest width: the sign bit is at bit 7 and the mask is 0xFF.
        auto p = ConstructProperty<SignedByteEnum>("test");
        ASSERT_TRUE(p);
        Property<int64_t> prop{p.GetProperty()};
        ASSERT_TRUE(prop);

        prop->SetValue(-1);
        EXPECT_EQ(prop->GetValue(), -1);
        EXPECT_EQ(p->GetValue(), SignedByteEnum::Neg);

        prop->SetValue(100);
        EXPECT_EQ(prop->GetValue(), 100);
        EXPECT_EQ(p->GetValue(), SignedByteEnum::Pos);
    }
}

/**
 * @tc.name: ArrayEnumSignedNegative
 * @tc.desc: SetAnyAt/GetAnyAt on the erased plain-enum array any (byte-buffer ByteArrayAnyImpl) round-trip a
 *           negative element correctly (int64 item view sign-extended, typed view truncated).
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, ArrayEnumSignedNegative, testing::ext::TestSize.Level1)
{
    // ArrayAnyType<SignedEnum> is the erased byte-buffer array any (via MapAnyType), unlike ArrayEnum<>.
    ArrayAnyType<SignedEnum> arr{BASE_NS::vector<SignedEnum>{SignedEnum::Pos, SignedEnum::Pos}};
    ASSERT_EQ(arr.GetSize(), 2u);

    auto item = arr.Clone({CloneValueType::DEFAULT_VALUE, TypeIdRole::ITEM});
    ASSERT_TRUE(item);

    // SetAnyAt a negative element via the item any
    ASSERT_TRUE(item->SetValue<int64_t>(-5));
    EXPECT_TRUE(arr.SetAnyAt(0, *item));

    // GetAnyAt reads it back: int64 item view is sign-extended, typed view truncated
    EXPECT_TRUE(arr.GetAnyAt(0, *item));
    EXPECT_EQ(GetValue<int64_t>(*item), -5);
    EXPECT_EQ(GetValue<SignedEnum>(*item), SignedEnum::Neg);

    // element 1 is untouched
    EXPECT_TRUE(arr.GetAnyAt(1, *item));
    EXPECT_EQ(GetValue<SignedEnum>(*item), SignedEnum::Pos);
}

// A plain builtin Any<Unsigned64Enum> (BaseTypedAny) whose GetData records the alignment of the buffer it
// is handed. SetAnyAt passes its internal scratch `tmp` to value.GetData(itemId, tmp, 8), so this probe
// observes that exact buffer without any instrumentation in the implementation.
class AlignmentProbeAny : public Any<Unsigned64Enum> {
public:
    using Any<Unsigned64Enum>::Any;
    AnyReturnValue GetData(const TypeId& id, void* data, size_t size) const override
    {
        if (reinterpret_cast<uintptr_t>(data) % alignof(Unsigned64Enum) != 0) {
            misaligned = true;
        }
        return Any<Unsigned64Enum>::GetData(id, data, size);
    }
    mutable bool misaligned = false;
};

/**
 * @tc.name: SetAnyAtFromPlainAny
 * @tc.desc: SetAnyAt on the erased byte-buffer array any (ByteArrayAnyImpl) fed a plain builtin Any<enum>
 *           (BaseTypedAny). SetAnyAt hands the value's GetData its internal `uint8_t tmp[8]`, and a builtin
 *           Any<enum>'s RawGet does *static_cast<Unsigned64Enum*>(tmp) — an 8-byte typed store that requires
 *           tmp to be 8-aligned (the erased item any uses memcpy and doesn't). The probe asserts tmp was
 *           aligned; the element must also round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, SetAnyAtFromPlainAny, testing::ext::TestSize.Level1)
{
    // Erased byte-buffer array any (ByteArrayAnyImpl via MapAnyType), unlike ArrayEnum<>. 64-bit underlying
    // -> ItemSize() == 8, so the typed store below is a full uint64_t-width write into tmp.
    ArrayAnyType<Unsigned64Enum> arr{BASE_NS::vector<Unsigned64Enum>{Unsigned64Enum::Lo, Unsigned64Enum::Lo}};
    ASSERT_EQ(arr.GetSize(), 2u);

    AlignmentProbeAny value{Unsigned64Enum::Hi};
    EXPECT_TRUE(arr.SetAnyAt(0, value));

    // The 8-byte typed store landed on SetAnyAt's tmp buffer, which must be 8-aligned.
    EXPECT_FALSE(value.misaligned);

    // element 0 now holds the value written through the typed cast; element 1 is untouched.
    auto item = arr.Clone({CloneValueType::DEFAULT_VALUE, TypeIdRole::ITEM});
    ASSERT_TRUE(item);
    EXPECT_TRUE(arr.GetAnyAt(0, *item));
    EXPECT_EQ(GetValue<Unsigned64Enum>(*item), Unsigned64Enum::Hi);
    EXPECT_TRUE(arr.GetAnyAt(1, *item));
    EXPECT_EQ(GetValue<Unsigned64Enum>(*item), Unsigned64Enum::Lo);
}

}  // namespace UTest
META_END_NAMESPACE()
