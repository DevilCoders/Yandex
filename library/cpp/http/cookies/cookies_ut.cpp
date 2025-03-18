#include "cookies.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TTestHttpCookies) {
    static const TStringBuf cookieString =
        "yyyyyyyyy=234; yandexUiD =449419021276887271; yabs-frequency = /3/zAuFPMm38Lhx36Li0o40//fGBR01o80O84W0811W460u420W441W00; fuid01=4c1bc0ef02adc24e.t9GFb_bjEBPv6KpW8rqujIpTlVZDrzjaSVkPBNsUYHc_yACsDSvOW-x6bnmjVeBt8rRYG0t775cRARhBqqCCMqjngbhaEo2BfebEYnAISNfbKpSkA6djZyvpTYnF4H22; L=EkkDbgZCR2JaY15eRFFSd3oNQ2VCYWRUNjcfX1oLJz0DSlx0MV8PXR99L3Q9IFg0VQh7MSUqF0UsI0wBei4GOg==.1281073600.7770.299251.cde4f6a5854638469566d6a903316a68; aw=1_teJzawcSAFTBiF8aqbvtiBoY3SgwMD32J1TUKkAELEL8Dht97RQaGObMR4rUa#tjAyw6Pmoae3zHwjItQPZrFNQs4iJLVC8empC4hgGQPHMDHP7Wicfct0EMwvm#Srtg1TkS7CwAAAP//AwDePiAj#A#; yandex_gid=54; spravka=dD0xMjgwNDA0ODExO2k9ODcuMjUwLjI0OS4yMzg7dT0xMjgwNDA0ODExMjU5NzUwMzk0O2g9YTAwOTk4ZjM0ODkxOWIxOTFkMGVkZTY1MTNkNjZiYjE=; t=p; yandex_login=alex-dude; Session_id=1281073600.892.0.35854780.2:91415892:220.8:1281073600262:1334748595:49.68658.57392.7496977416c140d1e61b8b09bc027a8e";

    static const TStringBuf cookieString2 =
        "yyyyyyyyy=234; yandexuid =345345345345; yabs-frequency = /3/zAuFPMm38Lhx36Li0o40//fGBR01o80O84W0811W460u420W441W00; fuid01=4c1bc0ef02adc24e.t9GFb_bjEBPv6KpW8rqujIpTlVZDrzjaSVkPBNsUYHc_yACsDSvOW-x6bnmjVeBt8rRYG0t775cRARhBqqCCMqjngbhaEo2BfebEYnAISNfbKpSkA6djZyvpTYnF4H22; L=EkkDbgZCR2JaY15eRFFSd3oNQ2VCYWRUNjcfX1oLJz0DSlx0MV8PXR99L3Q9IFg0VQh7MSUqF0UsI0wBei4GOg==.1281073600.7770.299251.cde4f6a5854638469566d6a903316a68; aw=1_teJzawcSAFTBiF8aqbvtiBoY3SgwMD32J1TUKkAELEL8Dht97RQaGObMR4rUa#tjAyw6Pmoae3zHwjItQPZrFNQs4iJLVC8empC4hgGQPHMDHP7Wicfct0EMwvm#Srtg1TkS7CwAAAP//AwDePiAj#A#; yandex_gid=54; spravka=dD0xMjgwNDA0ODExO2k9ODcuMjUwLjI0OS4yMzg7dT0xMjgwNDA0ODExMjU5NzUwMzk0O2g9YTAwOTk4ZjM0ODkxOWIxOTFkMGVkZTY1MTNkNjZiYjE=; t=p; yandex_login=alex-dude; Session_id=1281073600.892.0.35854780.2:91415892:220.8:1281073600262:1334748595:49.68658.57392.7496977416c140d1e61b8b09bc027a8e";

    static const TStringBuf smallCookieString = "yyyyyyyyy=234; yandexuid =345345345345";

    Y_UNIT_TEST(TestHttpCookies) {
        THttpCookies cookies(cookieString);

        UNIT_ASSERT_VALUES_EQUAL(cookies.Size(), 11);
        UNIT_ASSERT_VALUES_EQUAL(cookies.Get("yandexuid"), TStringBuf("449419021276887271"));
        UNIT_ASSERT_VALUES_EQUAL(cookies.Get("YandexUid"), TStringBuf("449419021276887271")); // case insensitive
        UNIT_ASSERT_VALUES_EQUAL(cookies.Get("yabs-frequency"), "/3/zAuFPMm38Lhx36Li0o40//fGBR01o80O84W0811W460u420W441W00");
        UNIT_ASSERT_VALUES_EQUAL(cookies.Get("session_id"), "1281073600.892.0.35854780.2:91415892:220.8:1281073600262:1334748595:49.68658.57392.7496977416c140d1e61b8b09bc027a8e");
        UNIT_ASSERT_VALUES_EQUAL(cookies.Get("noname"), TStringBuf());

        UNIT_ASSERT_VALUES_EQUAL(cookies.Has("aw"), true);
        UNIT_ASSERT_VALUES_EQUAL(cookies.Has("spravka"), true);
        UNIT_ASSERT_VALUES_EQUAL(cookies.Has("blah-blah"), false);

        cookies.Scan(cookieString2);
        UNIT_ASSERT_VALUES_EQUAL(cookies.Size(), 11);
        UNIT_ASSERT_VALUES_EQUAL(cookies.Get("yandexuid"), TStringBuf("345345345345"));
        UNIT_ASSERT_VALUES_EQUAL(cookies.Get("yabs-frequency"), "/3/zAuFPMm38Lhx36Li0o40//fGBR01o80O84W0811W460u420W441W00");
        UNIT_ASSERT_VALUES_EQUAL(cookies.Get("Session_id"), "1281073600.892.0.35854780.2:91415892:220.8:1281073600262:1334748595:49.68658.57392.7496977416c140d1e61b8b09bc027a8e");
        UNIT_ASSERT_VALUES_EQUAL(cookies.Get("noname"), TStringBuf());
    }

    Y_UNIT_TEST(TestCookiesToStringBasic) {
        THttpCookies cookies(smallCookieString);
        cookies.Add("t", "p");

        const TString expected = "yandexuid=345345345345; yyyyyyyyy=234; t=p";
        UNIT_ASSERT_VALUES_EQUAL(cookies.ToString(), expected);
    }

    Y_UNIT_TEST(TestCookiesToStringIdentity) {
        const THttpCookies cookies(cookieString);
        const auto str = cookies.ToString();
        const THttpCookies cookiesCopy(str);

        UNIT_ASSERT_VALUES_EQUAL(cookies.Size(), cookiesCopy.Size());
        for (const auto& [key, value] : cookies) {
            size_t c = cookies.NumOfValues(key);
            UNIT_ASSERT_VALUES_EQUAL(cookiesCopy.NumOfValues(key), c);
            for (size_t i=0; i < c; ++i) {
                UNIT_ASSERT_VALUES_EQUAL(cookies.Get(key, i), cookiesCopy.Get(key, i));
            }
        }
    }

    /*
    static const TStringBuf cookieString3 = "aaa=123; aaa=456";

    Y_UNIT_TEST(TestDoubleCookieTakeFirst) {
        THttpCookies cookies(cookieString3);

        UNIT_ASSERT_VALUES_EQUAL(cookies.Size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(cookies.Get("aaa"), TStringBuf("123"));
    }
    */
}
