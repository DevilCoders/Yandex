#include <kernel/snippets/schemaorg/software.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>

using namespace NSchemaOrg;

Y_UNIT_TEST_SUITE(TSoftwareApplicationTests) {
    Y_UNIT_TEST(TestFormatSnip) {
        TSoftwareApplication app;

        app.Description = UTF8ToWide("★★★ Only Windows 8 like Launcher with real live tiles - News, facebook, "
                                     "weather, stock market etc (Gmail, Twitter, Calender live tiles and many additional features are "
                                     "available in paid version) ★★★.");
        app.InteractionCount.push_back(u"UserWeeklyDownloads: 7,025");
        app.InteractionCount.push_back(u"UserDownloads: 106152");
        app.FileSize = u"12,81 МБ";
        app.OperatingSystem.push_back(u"Windows");
        app.OperatingSystem.push_back(u"iPhone, Android");
        app.Price = u"Free";
        app.PriceCurrency = u"foo";
        app.ApplicationSubCategory.push_back(u"Экшен");
        app.ApplicationSubCategory.push_back(u"Шутер");
        app.ApplicationSubCategory.push_back(u"Very long long long long long long long category");

        TUtf16String snip = app.FormatSnip("foo.ru", LANG_RUS);
        UNIT_ASSERT_EQUAL(UTF8ToWide(
                              "Бесплатно. "
                              "Размер: 13 Мб. "
                              "Более 100 000 скачиваний. "
                              "Windows, iOS, Android. "
                              "Категория: Экшен, Шутер. "
                              "Only Windows 8 like Launcher with real live tiles - News, facebook, weather, "
                              "stock market etc (Gmail, Twitter, Calender live tiles and many additional "
                              "features are available in paid version) ."),
                          snip);
    }
}
