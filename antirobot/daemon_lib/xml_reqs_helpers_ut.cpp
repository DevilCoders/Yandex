#include <library/cpp/testing/unittest/registar.h>

#include "xml_reqs_helpers.h"

namespace NAntiRobot{

Y_UNIT_TEST_SUITE(TestXmlReqsHelpers) {

Y_UNIT_TEST(TestNoXmlSearch) {
    static constexpr std::pair<TStringBuf, TStringBuf> TEST_DATA[] = {
        { "video-xmlsearch.yandex.com.tr", "yandex.com.tr" },
        { "video-xmlsearch.yandex.ru", "yandex.ru" },
        { "xmlsearch.hamster.yandex.com.tr", "hamster.yandex.com.tr" },
        { "xmlsearch.yandex.com.tr", "yandex.com.tr" },
        { "xmlsearch.yandex.ru", "yandex.ru" },
        { "seznam.xmlsearch.yandex.com", "yandex.com" },
        { "images.yandex.com", "images.yandex.com" },
        { "news.yandex.ru", "news.yandex.ru" },
    };

    for (size_t i = 0; i < Y_ARRAY_SIZE(TEST_DATA); ++i) {
        UNIT_ASSERT_STRINGS_EQUAL(NoXmlSearch(TEST_DATA[i].first), TEST_DATA[i].second);
    }
}

}

}
