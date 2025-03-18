/**
 *  Copyright (C) 2022 Huawei Technologies Co, Ltd.
 */
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <meta/api/make_callback.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/model/intf_data_model.h>

#include "testing_objects.h"

using namespace testing;

META_BEGIN_NAMESPACE()
namespace Test {

TEST(DataModelTest, ContainerDataModel)
{
    auto model = GetObjectRegistry().Create<IDataModel>(META_NS::ClassId::ContainerDataModel);
    ASSERT_TRUE(model);
    auto cont = interface_cast<IContainer>(model);
    ASSERT_TRUE(cont);

    size_t addIndex = -1;
    size_t RemoveIndex = -1;
    size_t moveFromIndex = -1;
    size_t moveToIndex = -1;

    model->OnDataAdded()->AddHandler(
        MakeCallback<IOnDataAdded>([&](auto index, auto count) { addIndex = index.Index(); }));
    model->OnDataRemoved()->AddHandler(
        MakeCallback<IOnDataRemoved>([&](auto index, auto count) { RemoveIndex = index.Index(); }));
    model->OnDataMoved()->AddHandler(MakeCallback<IOnDataMoved>([&](auto from, auto count, auto to) {
        moveFromIndex = from.Index();
        moveToIndex = to.Index();
    }));

    EXPECT_EQ(model->GetSize(), 0);

    auto o1 = CreateTestType<IObject>();
    cont->Add(o1);

    EXPECT_EQ(model->GetSize(), 1);
    EXPECT_EQ(addIndex, 0);

    EXPECT_EQ(model->GetModelData(DataModelIndex { 0 }), interface_pointer_cast<IMetadata>(o1));

    cont->Remove(cont->GetAt(0));

    EXPECT_EQ(model->GetSize(), 0);
    EXPECT_EQ(RemoveIndex, 0);

    cont->Add(o1);
    auto o2 = CreateTestType<IObject>();
    cont->Add(o2);
    auto o3 = CreateTestType<IObject>();
    cont->Add(o3);

    cont->Move(0, 2);
    EXPECT_EQ(model->GetSize(), 3);
    EXPECT_EQ(moveFromIndex, 0);
    EXPECT_EQ(moveToIndex, 2);

    EXPECT_EQ(model->GetModelData(DataModelIndex { 0 }), interface_pointer_cast<IMetadata>(o2));
    EXPECT_EQ(model->GetModelData(DataModelIndex { 1 }), interface_pointer_cast<IMetadata>(o3));
    EXPECT_EQ(model->GetModelData(DataModelIndex { 2 }), interface_pointer_cast<IMetadata>(o1));
}

} // namespace Test
META_END_NAMESPACE()
