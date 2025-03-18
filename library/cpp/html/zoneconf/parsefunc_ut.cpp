#include "parsefunc.h"

#include <library/cpp/html/face/propface.h>
#include <library/cpp/testing/unittest/registar.h>

class TParseFuncTest: public TTestBase {
    UNIT_TEST_SUITE(TParseFuncTest);
    UNIT_TEST(TestParseMetaRobots);
    UNIT_TEST(TestParseUrlLastname);
    UNIT_TEST(TestParseHttpCharset);
    UNIT_TEST(TestParseMetaYandex);
    UNIT_TEST_SUITE_END();

public:
    void TestParseMetaRobots();
    void TestParseUrlLastname();
    void TestParseHttpCharset();
    void TestParseMetaYandex();
};

#define PARSE_META_ROBOTS_ASSERT(ARG, RESULT) UNIT_ASSERT_EQUAL(parse_meta_robots((ARG), docProperties.Get()), TString(RESULT))

void TParseFuncTest::TestParseMetaRobots() {
    THolder<IParsedDocProperties> docProperties(CreateParsedDocProperties());
    PARSE_META_ROBOTS_ASSERT("", "xxxxx");
    PARSE_META_ROBOTS_ASSERT("NOINDE", "xxxxx");
    PARSE_META_ROBOTS_ASSERT("INDEX,ALL", "11xxx");
    PARSE_META_ROBOTS_ASSERT("NOINDEXX", "xxxxx");
    PARSE_META_ROBOTS_ASSERT("NONE", "00xxx");
    PARSE_META_ROBOTS_ASSERT("NoNe", "00xxx");
    PARSE_META_ROBOTS_ASSERT("NOINDEX", "0xxxx");
    PARSE_META_ROBOTS_ASSERT("NOFOLLOW", "x0xxx");
    PARSE_META_ROBOTS_ASSERT("NOARCHIVE", "xx0xx");
    PARSE_META_ROBOTS_ASSERT("ARCHIVE", "xx1xx");
    PARSE_META_ROBOTS_ASSERT("NOINDEX,NOFOLLOW", "00xxx");
    PARSE_META_ROBOTS_ASSERT("NOINDEX,NOINDEX", "0xxxx");
    PARSE_META_ROBOTS_ASSERT("NOINDEX,FOLLOW", "01xxx");
    PARSE_META_ROBOTS_ASSERT("NOODP", "xxx0x");
    PARSE_META_ROBOTS_ASSERT("NOYACA", "xxxx0");
    PARSE_META_ROBOTS_ASSERT("NOINDEX,NOYACA", "0xxx0");
    PARSE_META_ROBOTS_ASSERT("NOINDEX,NOODP", "0xx0x");
    docProperties->SetProperty(PP_ROBOTS, "0x0x0");
    PARSE_META_ROBOTS_ASSERT("", "0x0x0");
    docProperties->SetProperty(PP_YANDEX, "1xxx1");
    PARSE_META_ROBOTS_ASSERT("", "1x0x1");
    PARSE_META_ROBOTS_ASSERT("ALL", "110x1");
    PARSE_META_ROBOTS_ASSERT("NOINDEX", "1x0x1");
    docProperties->SetProperty(PP_ROBOTS, "xx1xx");
    PARSE_META_ROBOTS_ASSERT("NOARCHIVE", "1x1x1");
}

#define PARSE_META_YANDEX_ASSERT(ARG, RESULT) UNIT_ASSERT_EQUAL(parse_meta_yandex((ARG), docProperties.Get()), TString(RESULT))

void TParseFuncTest::TestParseMetaYandex() {
    THolder<IParsedDocProperties> docProperties(CreateParsedDocProperties());
    PARSE_META_YANDEX_ASSERT("", "xxxxx");
    PARSE_META_YANDEX_ASSERT("NOINDE", "xxxxx");
    PARSE_META_YANDEX_ASSERT("INDEX,ALL", "11xxx");
    PARSE_META_YANDEX_ASSERT("NOINDEXX", "xxxxx");
    PARSE_META_YANDEX_ASSERT("NONE", "00xxx");
    PARSE_META_YANDEX_ASSERT("NoNe", "00xxx");
    PARSE_META_YANDEX_ASSERT("NOINDEX", "0xxxx");
    PARSE_META_YANDEX_ASSERT("NOFOLLOW", "x0xxx");
    PARSE_META_YANDEX_ASSERT("NOARCHIVE", "xx0xx");
    PARSE_META_YANDEX_ASSERT("NOINDEX,NOFOLLOW", "00xxx");
    PARSE_META_YANDEX_ASSERT("NOINDEX,NOINDEX", "0xxxx");
    PARSE_META_YANDEX_ASSERT("NOINDEX,FOLLOW", "01xxx");
    PARSE_META_YANDEX_ASSERT("NOODP", "xxx0x");
    PARSE_META_YANDEX_ASSERT("NOYACA", "xxxx0");
    PARSE_META_YANDEX_ASSERT("NOINDEX,NOYACA", "0xxx0");
    PARSE_META_YANDEX_ASSERT("NOINDEX,NOODP", "0xx0x");
    docProperties->SetProperty(PP_ROBOTS, "0x0x0");
    PARSE_META_YANDEX_ASSERT("ALL", "11xxx");
    docProperties->SetProperty(PP_YANDEX, "1xxx1");
    PARSE_META_YANDEX_ASSERT("", "1xxx1");
    PARSE_META_YANDEX_ASSERT("NOFOLLOW", "10xx1");
    PARSE_META_YANDEX_ASSERT("NOINDEX", "1xxx1");
}

#define PARSE_URL_LASTNAME_ASSERT(ARG, RESULT) \
    UNIT_ASSERT_VALUES_EQUAL(parse_url_lastname((ARG), 0), RESULT)

void TParseFuncTest::TestParseUrlLastname() {
    PARSE_URL_LASTNAME_ASSERT("http://localhost/", "");
    PARSE_URL_LASTNAME_ASSERT("http://localhost/index", "index");
    PARSE_URL_LASTNAME_ASSERT("http://localhost/path///to//index.ext", "index.ext");
    // And this check looks incorrect to me yet reflects implementation
    PARSE_URL_LASTNAME_ASSERT("http://localhost/path///to//index.ext?id=2", "");
    // explicit error:
    PARSE_URL_LASTNAME_ASSERT("http://localhost", "localhost");
    PARSE_URL_LASTNAME_ASSERT("http://yandex.ru", "yandex.ru");
}

#define PARSE_HTTP_CHARSET_ASSERT(ARG, RESULT) \
    UNIT_ASSERT_VALUES_EQUAL(parse_http_charset((ARG), 0), RESULT)

void TParseFuncTest::TestParseHttpCharset() {
    PARSE_HTTP_CHARSET_ASSERT("text/html; charset=koi8-r", "KOI8-U");
    PARSE_HTTP_CHARSET_ASSERT("text/html; ChArsET \t\n\r=  \t\n\r koi8-r  \t\n\r", "KOI8-U");
    PARSE_HTTP_CHARSET_ASSERT("text/nonhtml; charset=koi8-r", "");
    PARSE_HTTP_CHARSET_ASSERT("text/html  ;   charset=SomeUnknownEncoding", "SomeUnknownEncoding");
    PARSE_HTTP_CHARSET_ASSERT("text/html; charset=  ", "");
}

UNIT_TEST_SUITE_REGISTRATION(TParseFuncTest);
