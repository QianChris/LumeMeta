/**
 *  Copyright (C) 2022 Huawei Technologies Co, Ltd.
 */
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <base/containers/unordered_map.h>
#include <core/io/intf_file_manager.h>

#include <meta/api/make_callback.h>
#include <meta/api/object.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/loaders/intf_file_content_loader.h>

using namespace testing;

META_BEGIN_NAMESPACE()
namespace Test {

MATCHER_P(HasProperty, name, "")
{
    const META_NS::IObject::Ptr& object = arg;
    auto meta = interface_cast<META_NS::IMetadata>(object);
    bool found = false;

    if (!meta || !object) {
        *result_listener << "where object type is invalid";
        return false;
    }

    for (auto prop : meta->GetProperties()) {
        if (prop->GetName() == name) {
            found = true;
            break;
        }
    }

    if (!found) {
        *result_listener << "where object did not have property '" << name.data() << "'";
    }
    return found;
}

constexpr BASE_NS::string_view TRANSLATION_STRING_KEY = "strings";

class UnitTest_CsvLoaderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        loader_ = META_NS::GetObjectRegistry().Create<IFileContentLoader>(ClassId::CsvStringResourceLoader);
        ASSERT_NE(loader_, nullptr);
    }

    using CsvContentItemType = BASE_NS::pair<BASE_NS::string, BASE_NS::vector<BASE_NS::string>>;
    using CsvContentType = BASE_NS::vector<CsvContentItemType>;

    void CheckTranslations(
        const BASE_NS::string& path, const BASE_NS::vector<BASE_NS::string>& keys, const CsvContentType& content)
    {
        loader_->SetFile(CORE_NS::GetPluginRegister().GetFileManager().OpenFile(path));
        Object object(loader_->Create({}));
        ASSERT_TRUE(object);
        ASSERT_NE(object.GetPtr(), nullptr);

        // Object should have a "strings" property
        ASSERT_THAT(object, HasProperty(TRANSLATION_STRING_KEY));
        auto stringsProp = Metadata(object).GetProperty<IObject::Ptr>(TRANSLATION_STRING_KEY);
        ASSERT_TRUE(stringsProp);
        Object strings(stringsProp->GetValue());
        ASSERT_TRUE(strings);
        EXPECT_EQ(Metadata(strings).GetProperties().size(), content.size());

        // Go through our expected content and chech that the object has all of the languages and
        // corresponding strings
        for (const auto& item : content) {
            BASE_NS::string_view lang(item.first);
            ASSERT_THAT(strings, HasProperty(lang));

            auto translationsProp = Metadata(strings).GetProperty<IObject::Ptr>(lang);
            ASSERT_TRUE(translationsProp);
            Object translations(translationsProp->GetValue());
            ASSERT_TRUE(translations);

            // Must contain expected amount of translation strings
            const auto& expectedTranslations = item.second;
            ASSERT_EQ(Metadata(translations).GetProperties().size(), expectedTranslations.size());
            ASSERT_EQ(keys.size(), expectedTranslations.size());

            // Check that each translation found in the object matches our expected translation table
            for (size_t i = 0; i < expectedTranslations.size(); i++) {
                auto prop = Metadata(translations).GetProperty<BASE_NS::string>(keys[i]);
                ASSERT_TRUE(prop) << "lang: " << item.first.data() << ", index: " << i;
                ASSERT_EQ(prop->GetValue(), expectedTranslations[i])
                    << "lang: " << item.first.data() << ", index: " << i;
            }
        }
    }

    IFileContentLoader::Ptr loader_;
};

TEST_F(UnitTest_CsvLoaderTest, Basic)
{
    constexpr auto path = "assets://csv/basic.csv";
    static BASE_NS::vector<BASE_NS::string> keys = { "First string", "Second string", "Click me" };
    static CsvContentType content = { { "fi", { "Eka merkkijono", "Toka merkkijono", "Klikkaa mua" } },
        { "en", { "First string", "Second string", "Click me" } },
        { "zh", { "第一个字符串", "第二个字符串", "点我" } } };

    CheckTranslations(path, keys, content);
}

TEST_F(UnitTest_CsvLoaderTest, Quotes)
{
    constexpr auto path = "assets://csv/quotes.csv";
    // The CSV file contains quoted strings with commas, double quotes and also leading and
    // trailing whitespace between opening/closing quote and the delimiter. Those should be
    // handled correctly.
    static BASE_NS::vector<BASE_NS::string> keys = { "Click,me", "Click,\"12" };
    static CsvContentType content = { { "fi", { "Klikkaa,mua", "Klikkaa,\"12" } },
        { "en", { "Click,me", "Click,\"12" } }, { "zh", { "点,我", "点,12" } } };

    CheckTranslations(path, keys, content);
}

TEST_F(UnitTest_CsvLoaderTest, Multiline)
{
    constexpr auto path = "assets://csv/multiline.csv";
    // These strings/keys should match the values in above csv file
    static BASE_NS::vector<BASE_NS::string> keys = { "First string", "Click me" };
    static CsvContentType content = { { "fi", { "Eka \nmerkkijono", "Klikkaa mua" } },
        { "en", { "First \nstring", "Click me" } }, { "zh", { "第一\n个字符串", "点我" } } };

    CheckTranslations(path, keys, content);
}

TEST_F(UnitTest_CsvLoaderTest, Complex)
{
    constexpr auto path = "assets://csv/complex.csv";
    // These strings/keys should match the values in above csv file
    static BASE_NS::vector<BASE_NS::string> keys = { "1991", "1992", "1993", "1994" };
    static CsvContentType content = { { "make", { "Ford", "Chevy", "Chevy", "Jeep" } },
        { "model",
            { "E350", "Venture \"Extended Edition\"", "Venture \"Extended Edition, Very Large\"", "Grand Cherokee" } },
        { "description", { "ac, abs, moon", "", "", "MUST SELL!\nair, moon roof, loaded" } },
        { "price", { "3000.00", "4900.00", "5000.00", "4799.00" } } };

    CheckTranslations(path, keys, content);
}

TEST_F(UnitTest_CsvLoaderTest, CsvInvalid)
{
    constexpr auto path1 = "assets://csv/invalid_content.csv";
    constexpr auto path2 = "assets://csv/invalid_structure.csv";
    constexpr auto path3 = "assets://csv/invalid_header.csv";
    constexpr auto path4 = "assets://csv/invalid_empty.csv";
    constexpr auto path5 = "assets://csv/invalid_file_doesnt_exist.csv";
    int changed = 0;

    loader_->ContentChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([&changed] { changed++; }));

    loader_->SetFile(CORE_NS::GetPluginRegister().GetFileManager().OpenFile(path1));
    EXPECT_EQ(loader_->Create({}), nullptr);
    loader_->SetFile(CORE_NS::GetPluginRegister().GetFileManager().OpenFile(path2));
    EXPECT_EQ(loader_->Create({}), nullptr);
    loader_->SetFile(CORE_NS::GetPluginRegister().GetFileManager().OpenFile(path3));
    EXPECT_EQ(loader_->Create({}), nullptr);
    loader_->SetFile(CORE_NS::GetPluginRegister().GetFileManager().OpenFile(path4));
    EXPECT_EQ(loader_->Create({}), nullptr);
    loader_->SetFile(CORE_NS::GetPluginRegister().GetFileManager().OpenFile(path5));
    EXPECT_EQ(loader_->Create({}), nullptr);

    EXPECT_EQ(changed, 5);
}

} // namespace Test
META_END_NAMESPACE()
