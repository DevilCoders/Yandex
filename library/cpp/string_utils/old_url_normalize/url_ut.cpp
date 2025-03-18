#include "url.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TUtilUrlTest) {
    Y_UNIT_TEST(TestStrongNormalizeUrl) {
        UNIT_ASSERT_VALUES_EQUAL("ya.ru", StrongNormalizeUrl("http://ya.ru"));
        UNIT_ASSERT_VALUES_EQUAL("ya.ru", StrongNormalizeUrl("http://ya.ru"));
        UNIT_ASSERT_VALUES_EQUAL("ya.ru", StrongNormalizeUrl("https://www.ya.ru"));

        UNIT_ASSERT_VALUES_EQUAL("ya.ru/q=some%20query", StrongNormalizeUrl("http://ya.ru/q=some+query"));
        UNIT_ASSERT_VALUES_EQUAL("ya.ru/some%09path", StrongNormalizeUrl("ya.ru/some%09path"));

        {
            //this is not valid normalization, but declare here current url.h StrongNormalizeUrl status quo (may be fixed some time)
            UNIT_ASSERT_VALUES_EQUAL("youtube.com/watch?v=9bzkp7q19f0", StrongNormalizeUrl("youtube.com/watch?v=9bZkp7q19f0"));                                //cgi parameter to lower (make link invalid)
            UNIT_ASSERT_VALUES_EQUAL("ya.ru/some%20path", StrongNormalizeUrl("ya.ru/some+path"));                                                              // '+' in non-cgi converted to '%20'
            UNIT_ASSERT_VALUES_EQUAL("ya.ru/s%6fmepath", StrongNormalizeUrl("ya.ru/s%6fmepath"));                                                              //%6f is not converted to 'o'
            UNIT_ASSERT_VALUES_EQUAL("%D1%80%D1%9F%D1%80%D1%9B%D1%80%C2%A7%D1%80%D0%86%D1%80%D1%92.%D1%80%C2%A0%D1%80%D2%91", StrongNormalizeUrl("ПОЧТА.РФ")); // utf-8 hostnames are not handled
            UNIT_ASSERT_VALUES_EQUAL("ftp://xxx.ru", StrongNormalizeUrl("ftp://xxx.ru"));                                                                      // this scheme is not handled now
            UNIT_ASSERT_VALUES_EQUAL("svn%20ssh://arcadia.yandex-team.ru", StrongNormalizeUrl("svn+ssh://arcadia.yandex-team.ru"));                            // just funny
        }
    }
}
