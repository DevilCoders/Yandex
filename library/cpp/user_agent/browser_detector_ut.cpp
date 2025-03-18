#include <library/cpp/testing/unittest/registar.h>

#include "browser_detector.h"

#include <util/stream/file.h>

struct TTest {
    TString UserAgent;
    TString Browser;
    TString Version;
    TString OsFamily;
    TString OsVersion;
};

void TestBrowsers(const TTest* tests, const size_t size) {
    for (size_t i = 0; i < size; ++i) {
        {
            TString browser = NUserAgent::TBrowserDetector::Instance().GetBrowser(tests[i].UserAgent);
            UNIT_ASSERT_STRINGS_EQUAL_C(browser, tests[i].Browser, tests[i].UserAgent);
        }
        {
            TString version;
            TString browser = NUserAgent::TBrowserDetector::Instance().GetBrowser(tests[i].UserAgent, &version);
            UNIT_ASSERT_STRINGS_EQUAL_C(browser, tests[i].Browser, tests[i].UserAgent);
            UNIT_ASSERT_STRINGS_EQUAL_C(version, tests[i].Version, tests[i].UserAgent);
        }
    }
}

void TestOS(const TTest* tests, const size_t size) {
    for (size_t i = 0; i < size; ++i) {
        {
            TString osFamily = NUserAgent::TBrowserDetector::Instance().GetOS(tests[i].UserAgent);
            UNIT_ASSERT_STRINGS_EQUAL_C(osFamily, tests[i].OsFamily, tests[i].UserAgent);
        }
        {
            TString osVersion;
            TString osFamily = NUserAgent::TBrowserDetector::Instance().GetOS(tests[i].UserAgent, &osVersion);
            UNIT_ASSERT_STRINGS_EQUAL_C(osFamily, tests[i].OsFamily, tests[i].UserAgent);
            UNIT_ASSERT_STRINGS_EQUAL_C(osVersion, tests[i].OsVersion, tests[i].UserAgent);
        }
    }
}

Y_UNIT_TEST_SUITE(TBrowserDetectorTest) {
    Y_UNIT_TEST(PopularBrowsers) {
        const TTest tests[] = {
            {"Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; MRA 5.5 (build 02842); .NET CLR 2.0.50727; .NET CLR 3.0.4506.2152; .NET CLR 3.5.30729)",
             "MSIE", "6.0", "", ""},
            {"Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 6.1; Trident/4.0; SLCC2; .NET CLR 2.0.50727; .NET CLR 3.5.30729; .NET CLR 3.0.30729; Media Center PC 6.0)",
             "MSIE", "7.0", "", ""},
            {"Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0; MRA 6.0 (build 5831); MRSPUTNIK 2, 4, 1, 44; SLCC2; .NET CLR 2.0.50727; .NET CLR 3.5.30729; .NET CLR 3.0.30729; Media Center PC 6.0)",
             "MSIE", "8.0", "", ""},
            {"Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0)",
             "MSIE", "9.0", "", ""},
            {"Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.2; WOW64; Trident/6.0)",
             "MSIE", "10.0", "", ""},

            {"Mozilla/5.0 (Windows NT 6.1; WOW64; rv:18.0) Gecko/20100101 Firefox/18.0",
             "Firefox", "18.0", "", ""},
            {"Mozilla/5.0 (Windows NT 6.1; WOW64; rv:9.0.1) Gecko/20100101 Firefox/9.0.1",
             "Firefox", "9.0", "", ""},
            {"Mozilla/5.0 (Macintosh; Intel Mac OS X 10.6; rv:18.0) Gecko/20100101 Firefox/18.0",
             "Firefox", "18.0", "", ""},
            {"Mozilla/5.0 (X11; Ubuntu; Linux i686; rv:11.0) Gecko/20100101 Firefox/11.0",
             "Firefox", "11.0", "", ""},

            {"Opera/9.80 (Windows NT 6.1; U; ru) Presto/2.10.289 Version/12.02",
             "Opera", "12.02", "", ""},
            {"Opera/9.80 (Windows NT 5.1; MRA 6.0 (build 5976)) Presto/2.12.388 Version/12.12",
             "Opera", "12.12", "", ""},
            {"Opera/9.80 (Windows NT 6.1; WOW64; U; MRA 6.0 (build 6005); ru) Presto/2.10.229 Version/11.64",
             "Opera", "11.64", "", ""},
            {"Opera/9.80 (Macintosh; Intel Mac OS X 10.7.5; U; ru) Presto/2.10.289 Version/12.01",
             "Opera", "12.01", "", ""},
            {"Opera/9.80 (X11; Linux i686) Presto/2.12.388 Version/12.12",
             "Opera", "12.12", "", ""},

            {"Opera/9.80 (Android; Opera Mini/7.5.32193/28.3590; U; ru) Presto/2.8.119 Version/11.10",
             "OperaMini", "7.5", "", ""},
            {"Opera/9.80 (iPhone; Opera Mini/5.0/28.3590; U; ru) Presto/2.8.119 Version/11.10",
             "OperaMini", "5.0", "", ""},
            {"Opera/9.80 (J2ME/MIDP; Opera Mini/5.1.21214/28.3590; U; ru) Presto/2.8.119 Version/11.10",
             "OperaMini", "5.1", "", ""},
            {"Opera/9.80 (Series 60; Opera Mini/7.0.29974/28.3590; U; ru) Presto/2.8.119 Version/11.10",
             "OperaMini", "7.0", "", ""},

            {"Opera/9.80 (Android 2.3.6; Linux; Opera Mobi/ADR-1212030820) Presto/2.11.355 Version/12.10",
             "OperaMobile", "9.80", "", ""},
            {"Opera/9.80 (S60; SymbOS; Opera Mobi/SYB-1202242143; U; ru) Presto/2.10.254 Version/12.00",
             "OperaMobile", "9.80", "", ""},
            {"Opera/9.80 (Windows Mobile; WCE; Opera Mobi/WMD-50433; U; en) Presto/2.4.13 Version/10.00",
             "OperaMobile", "9.80", "", ""},

            {"Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.17 (KHTML, like Gecko) Chrome/24.0.1312.56 Safari/537.17",
             "GoogleChrome", "24.0", "", ""},
            {"Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.17 (KHTML, like Gecko) Chrome/24.0.1312.56 Safari/537.17",
             "GoogleChrome", "24.0", "", ""},
            {"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) AppleWebKit/537.17 (KHTML, like Gecko) Chrome/24.0.1312.56 Safari/537.17",
             "GoogleChrome", "24.0", "", ""},
            {"Mozilla/5.0 (X11; Linux i686) AppleWebKit/535.19 (KHTML, like Gecko) Ubuntu/12.04 Chromium/18.0.1025.168 Chrome/18.0.1025.168 Safari/535.19",
             "GoogleChrome", "18.0", "", ""},

            {"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_8) AppleWebKit/537.4 (KHTML, like Gecko) Chrome/22.0.1104.221 YaBrowser/1.5.1104.221 Safari/537.4",
             "YandexBrowser", "1.5", "", ""},
            {"Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.4 (KHTML, like Gecko) Chrome/22.0.1104.222 YaBrowser/1.5.1104.222 Safari/537.4",
             "YandexBrowser", "1.5", "", ""},

            {"Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.47 Safari/535.11 MRCHROME",
             "InternetMailRu", "17.0", "", ""},
            {"Mozilla/5.0 (Windows NT 5.1) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.47 Safari/535.11 MRCHROME",
             "InternetMailRu", "17.0", "", ""},

            {"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_8) AppleWebKit/534.57.2 (KHTML, like Gecko) Version/5.1.7 Safari/534.57.2",
             "Safari", "5.1", "", ""},

            {"Mozilla/5.0 (iPhone; CPU iPhone OS 6_0_1 like Mac OS X) AppleWebKit/536.26 (KHTML, like Gecko) Version/6.0 Mobile/10A523 Safari/8536.25",
             "MobileSafari", "6.0", "", ""},
            {"Mozilla/5.0 (iPad; U; CPU OS 4_3_3 like Mac OS X; ru-ru) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8J2 Safari/6533.18.5",
             "MobileSafari", "5.0", "", ""},

            {"Mozilla/5.0 (Android; Mobile; rv:27.0) Gecko/27.0 Firefox/27.0",
             "Firefox", "27.0", "", ""},
            {"Mozilla/5.0 (Linux; U; Android 4.0.3; ru-ru; U9500 Build/HuaweiU9500) AppleWebKit/534.30 (KHTML, like Gecko) Version/4.0 Mobile Safari/53",
             "AndroidBrowser", "4.0", "", ""},
            {"Mozilla/5.0 (Linux; Android 4.0.3; U9500 Build/HuaweiU9500) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/32.0.1700.99 Mobile Safari/537.36",
             "GoogleChrome", "32.0", "", ""},
            {"Mozilla/5.0 (Linux; Android 4.0.3; U9500 Build/HuaweiU9500) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/32.0.1700.102 YaBrowser/14.2.1700.12147.00 Mobile Safari/537.36",
             "YandexBrowser", "14.2", "", ""},

            {"SomeUnknownBrowser/1.0", "Unknown", "", "", ""},
            {"", "Unknown", "", "", ""}};

        TestBrowsers(tests, Y_ARRAY_SIZE(tests));

        const TTest testsUatraits[] = {
            {"Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/37.0.2062.122 Safari/537.36 OPR/24.0.1558.64 (Edition Campaign 34)",
             "Opera", "24.0.1558.64", "Windows", "6.1"}};

        const bool initialized = NUserAgent::TBrowserDetector::Instance().InitializeUatraits("browser.xml");
        UNIT_ASSERT(initialized);
        TestBrowsers(testsUatraits, Y_ARRAY_SIZE(testsUatraits));

        TFileInput browserInput{"browser.xml"};
        TString browserData = browserInput.ReadAll();
        const bool initializedFromInput = NUserAgent::TBrowserDetector::Instance().InitializeUatraitsFromMemory(browserData);
        UNIT_ASSERT(initializedFromInput);
        TestBrowsers(testsUatraits, Y_ARRAY_SIZE(testsUatraits));
        TestOS(testsUatraits, Y_ARRAY_SIZE(testsUatraits));
    }
}
