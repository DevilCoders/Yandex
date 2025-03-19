#include <kernel/common_server/library/metasearch/helpers/attribute.h>

#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/string_utils/quote/quote.h>

using NUtil::GetAttributeValue;

Y_UNIT_TEST_SUITE(AttributeHelper) {
    Y_UNIT_TEST(Brackets) {
        UNIT_ASSERT_EQUAL(GetAttributeValue("url", "fake    url:\"abc\"   url2:(cba)  "), "abc");
        UNIT_ASSERT_EQUAL(GetAttributeValue("url", "url:(abzzzzzzzzzz)   url2:(cba)  "), "abzzzzzzzzzz");
        UNIT_ASSERT_EQUAL(GetAttributeValue("url", "fake    url:'abc1'"), "abc1");

        const TString str = CGIUnescapeRet("%28%25request%25%29%20softness%3A99%20country%3A%22RU%22%20content_type%3A%22app%22%20store%3A%22GPLAY%22%20");
        UNIT_ASSERT_EQUAL(GetAttributeValue("store", str), "GPLAY");
        UNIT_ASSERT_EQUAL(GetAttributeValue("country", str), "RU");
        UNIT_ASSERT_EQUAL(GetAttributeValue("softness", str), "99");
        UNIT_ASSERT_EQUAL(GetAttributeValue("content_type", str), "app");

        UNIT_ASSERT_EQUAL(GetAttributeValue("host", "host:(yandex.ru)"), "yandex.ru");
    }

    Y_UNIT_TEST(Free) {
        UNIT_ASSERT_EQUAL(GetAttributeValue("url", "url:yandex.ru google.com"), "yandex.ru");
    }

    Y_UNIT_TEST(Incorrect) {
        UNIT_ASSERT_EQUAL(GetAttributeValue("url", "url:(yandex.ru google.com"), "yandex.ru google.com");
    }
}
