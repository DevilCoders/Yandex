#include <library/cpp/testing/unittest/registar.h>

#include "config.h"
#include "config_global.h"
#include "req_types.h"

#include <antirobot/daemon_lib/ut/utils.h>

namespace NAntiRobot {
class TTestReqTypesParams : public TTestBase {
public:
    void SetUp() override {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString("<Daemon>\n"
                                                       "FormulasDir = .\n"
                                                       "</Daemon>");
        TJsonConfigGenerator jsonConf;
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig.LoadFromString(jsonConf.Create(), GetJsonServiceIdentifierStr());
    }
};

Y_UNIT_TEST_SUITE_IMPL(TestReqTypes, TTestReqTypesParams) {

Y_UNIT_TEST(TestHostToHostType) {
    // HOST_VIDEO
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("beta.yandex.ru", "/video", ""), HOST_VIDEO);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandsearch.yandex.ru", "/video", ""), HOST_VIDEO);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ru", "/video", ""), HOST_VIDEO);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("www.yandex.com.tr", "/video", ""), HOST_VIDEO);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("video.yandex.ru"), HOST_VIDEO);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("m.video.yandex.ru"), HOST_VIDEO);

    // HOST_WEB
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("beta.yandex.ru"), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandsearch.yandex.ru"), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("copy.yandex.net"), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("people.yandex.ru"), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("phone-search.yandex.ru"), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("rca.yandex.ru"), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("something.rca.yandex.ru"), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("rrsearch.yandex.ru"), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ru", "/search", ""), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("www.yandex.ru", "/search", ""), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.by", "/search", ""), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.kz", "/search", ""), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ru", "/search", ""), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.st", "/search", ""), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ua", "/search", ""), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("beta.yandex.ru", "/yandsearch", ""), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ru", "/search", "ui=webmobileapp.yandex"), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ru", "/search", "ui=webmobileapp.yandex&some_text"), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ru", "/search", "some_text&ui=webmobileapp.yandex"), HOST_WEB);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ru/search?ui=webmobileapp.yandex"), HOST_WEB);


    // HOST_MORDA
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("www.yandex.ru"), HOST_MORDA);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.by"), HOST_MORDA);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.kz"), HOST_MORDA);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ru"), HOST_MORDA);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.st"), HOST_MORDA);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ua"), HOST_MORDA);

    // HOST_YARU
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("ya.ru"), HOST_YARU);

    // HOST_XMLSEARCH_COMMON
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("video-xmlsearch.yandex.ru"), HOST_XMLSEARCH_COMMON);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("xmlsearch.yandex.ru"), HOST_XMLSEARCH_COMMON);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("seznam.xmlsearch.yandex.com"), HOST_XMLSEARCH_COMMON);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("xmlsearch.yandex.com.tr"), HOST_XMLSEARCH_COMMON);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("xmlsearch.hamster.yandex.com.tr"), HOST_XMLSEARCH_COMMON);

    // HOST_CLICK
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("clck.yandex.com"), HOST_CLICK);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("clck.yandex.ru"), HOST_CLICK);

    // HOST_IMAGES
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("images.yandex.by"), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("images.yandex.kz"), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("images.yandex.ru"), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("images.yandex.ua"), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("images.yandex.com"), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("images.yandex.com.tr"), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("m.images.yandex.by"), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("m.images.yandex.ru"), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("m.images.yandex.ua"), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("gorsel.yandex.com.tr"), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("m.gorsel.yandex.com.tr"), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("m.images.yandex.com"), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("beta.yandex.ru", "/images", ""), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandsearch.yandex.ru", "/images", ""), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ru", "/images", ""), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.com.tr", "/images", ""), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.com.tr", "/gorsel", ""), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("www.yandex.com.tr", "/images", ""), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("www.images.yandex.com.tr"), HOST_IMAGES);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("www.yandex.com.tr", "/images", ""), HOST_IMAGES);

    // HOST_NEWS
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("m.news.yandex.ru"), HOST_NEWS);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("news.yandex.by"), HOST_NEWS);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("news.yandex.kz"), HOST_NEWS);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("news.yandex.ru"), HOST_NEWS);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("news.yandex.ua"), HOST_NEWS);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("pda.news.yandex.by"), HOST_NEWS);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("pda.news.yandex.ru"), HOST_NEWS);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("pda.news.yandex.ua"), HOST_NEWS);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("wap.news.yandex.ru"), HOST_NEWS);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("wap.news.yandex.by"), HOST_NEWS);

    // HOST_YACA
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("search.yaca.yandex.ru"), HOST_YACA);

    // HOST_BLOGS
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("blogs.yandex.by"), HOST_BLOGS);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("blogs.yandex.ru"), HOST_BLOGS);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("blogs.yandex.ua"), HOST_BLOGS);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("m.blogs.yandex.ru"), HOST_BLOGS);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("m.blogs.yandex.ua"), HOST_BLOGS);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("pda.blogs.yandex.ru"), HOST_BLOGS);

    // HOST_HILIGHTER
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("hghltd.yandex.net"), HOST_HILIGHTER);

    // HOST_MARKET
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("market.yandex.ru"), HOST_MARKET);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("market.yandex.com.tr"), HOST_MARKET);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("m.market.yandex.com"), HOST_MARKET);

    // HOST_RCA
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("rca.yandex.com"), HOST_RCA);

    // HOST_BUS
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ru", "/bus", ""), HOST_BUS);

    // HOST_POGODA
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.com.am", "/weather", ""), HOST_POGODA);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.com.ge", "/weather", ""), HOST_POGODA);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.co.il", "/weather", ""), HOST_POGODA);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.com", "/weather", ""), HOST_POGODA);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ru", "/pogoda", ""), HOST_POGODA);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.com.tr", "/hava", ""), HOST_POGODA);

    // HOST_SEARCHAPP for testing query
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ru", "/video/search", "some_text&ui=webmobileapp.yandex"), HOST_SEARCHAPP);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ru/searchapp?some_text"), HOST_WEB);

    // HOST_COLLECTIONS
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ru", "/collections/api/v0.1/link-status", "source_name=turbo&ui=touch"), HOST_COLLECTIONS);
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("collections.yandex.ru", "/collections/api/v0.1/recent-timestamp", "source_name=yabrowser&source_version=20.3.0.1223&ui=desktop"), HOST_COLLECTIONS);

    // HOST_TUTOR
    UNIT_ASSERT_VALUES_EQUAL(HostToHostType("yandex.ru", "/tutor", ""), HOST_TUTOR);
}

}

}
