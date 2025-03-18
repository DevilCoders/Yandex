#include <library/cpp/testing/unittest/registar.h>

#include "config_global.h"
#include "fullreq_info.h"
#include "reloadable_data.h"
#include "return_path.h"

#include <antirobot/daemon_lib/ut/utils.h>

#include <antirobot/lib/keyring.h>

#include <library/cpp/http/server/response.h>

#include <util/network/address.h>
#include <util/stream/str.h>
#include <util/string/printf.h>
#include <library/cpp/string_utils/quote/quote.h>

namespace NAntiRobot {

    class TestReturnPathParams : public TTestBase {
        void SetUp() override {
            ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString("<Daemon>\n"
                                                           "CaptchaApiHost = ::\n"
                                                           "FormulasDir = .\n"
                                                           "CbbApiHost = ::\n"
                                                           "</Daemon>\n"
                                                           "<Zone></Zone>");
            TJsonConfigGenerator jsonConf;
            ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig.LoadFromString(jsonConf.Create(), GetJsonServiceIdentifierStr());
            const TString testKey = "102c46d700bed5c69ed20b7473886468";
            TStringInput keys(testKey);
            TKeyRing::SetInstance(TKeyRing(keys));
        }
    };

Y_UNIT_TEST_SUITE_IMPL(TestReturnPath, TestReturnPathParams) {

namespace {

TAutoPtr<TRequest> CreateRequest(const TString& request) {
    TStringInput input(request);
    THttpInput httpInput(&input);

    return new TFullReqInfo(httpInput, "", "", TReloadableData(), TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
}

}

Y_UNIT_TEST(TestRetPathFromRequest) {
    const TString REQUEST = "GET /yandsearch?text=some_text<and>some%0d%0achars\" HTTP/1.0\r\n"
                           "Accept: */*, text/html, application/xhtml+xml\r\n"
                           "User-Agent: Mozilla/4.0 (compatible; MSIE 7.0b; Windows NT 6.0 ; .NET CLR 2.0.50215; SL Commerce Client v1.0; Tablet PC 2.0\r\n"
                           "Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r\n"
                           "Accept-Charset: windows-1251,utf-8;q=0.7,*;q=0.3\r\n"
                           "Host: yandex.com.tr\r\n"
                           "Proxy-Connection: Keep-Alive\r\n"
                           "Connection: Keep-Alive\r\n"
                           "Keep-Alive: 361\r\n"
                           "Pragma: no-cache\r\n"
                           "X-Forwarded-For-Y: 188.186.226.2\r\n"
                           "X-Source-Port-Y: 10769\r\n"
                           "X-Req-Id: 1354193658048134-768518413483003042\r\n"
                           "\r\n";
    TAutoPtr<TRequest> req = CreateRequest(REQUEST);

    UNIT_ASSERT_NO_EXCEPTION(TReturnPath::FromRequest(*req));
    TReturnPath rp = TReturnPath::FromRequest(*req);

    UNIT_ASSERT_STRINGS_EQUAL(rp.GetURL(), "http://yandex.com.tr/yandsearch?text=some_text<and>some%0d%0achars\"");
    TCgiParameters cgi;
    rp.AddToCGI(cgi);

    UNIT_ASSERT_NO_EXCEPTION(TReturnPath::FromCgi(cgi));
    TReturnPath rpFromCgi = TReturnPath::FromCgi(cgi);
    UNIT_ASSERT_STRINGS_EQUAL(rp.GetURL(), rpFromCgi.GetURL());
}

Y_UNIT_TEST(TestRetPathFromHeader) {
    const char* REQUEST = "GET /yandsearch?text=some_text<and>some%%0d%%0achars\" HTTP/1.0\r\n"
                           "Accept: */*, text/html, application/xhtml+xml\r\n"
                           "User-Agent: Mozilla/4.0 (compatible; MSIE 7.0b; Windows NT 6.0 ; .NET CLR 2.0.50215; SL Commerce Client v1.0; Tablet PC 2.0\r\n"
                           "Host: yandex.com.tr\r\n"
                           "X-Forwarded-For-Y: 188.186.226.2\r\n"
                           "X-Source-Port-Y: 10769\r\n"
                           "X-Req-Id: 1354193658048134-768518413483003042\r\n"
                           "X-Retpath-Y: %s\r\n"
                           "\r\n";

    const char* validRetPaths[] = {
        "http://yandex.ru/retpath",
        "https://yandex.ru/",
        "http://img.yandex.ru",
        "http://some-service.yandex.ru/retpath",
    };

    for (const auto& retPath: validRetPaths) {
        THolder<TRequest> req = CreateRequest(Sprintf(REQUEST, retPath));

        UNIT_ASSERT_NO_EXCEPTION(TReturnPath::FromRequest(*req));
        TReturnPath rp = TReturnPath::FromRequest(*req);

        UNIT_ASSERT_STRINGS_EQUAL(rp.GetURL(), retPath);
    }
    {
        const char* retPath = "http://not-a-yandex.ru/retpath";
        TAutoPtr<TRequest> req = CreateRequest(Sprintf(REQUEST, retPath));

        UNIT_ASSERT_EXCEPTION(TReturnPath::FromRequest(*req), TReturnPath::TInvalidRetPathException);
    }
}

Y_UNIT_TEST(TestRetPathFromCgiWithBase64) {
    {
        TCgiParameters cgi;
        cgi.InsertUnescaped("retpath", "aHR0cDovL3lhbmRleC5jb20udHIveWFuZHNlYXJjaD90ZXh0PWNhdHMmbHI9MTMw_9d129157cd3fb9b1f1df3765def55d1");
        UNIT_ASSERT_EXCEPTION(TReturnPath::FromCgi(cgi), TReturnPath::TInvalidRetPathException);
    }
    {
        TCgiParameters cgi;
        cgi.InsertUnescaped("retpath", "aHR0cDovL3lhbmRleC5jb20udHIveWFuZHNlYXJjaD90ZXh0PXNvbWVfdGV4dDxhbmQ+c29tZSUwZCUwYWNoYXJzIg==_145897882dc530b256355ee423c1c115");
        UNIT_ASSERT_NO_EXCEPTION(TReturnPath::FromCgi(cgi));
        TReturnPath rpFromCgi = TReturnPath::FromCgi(cgi);
        UNIT_ASSERT_STRINGS_EQUAL("http://yandex.com.tr/yandsearch?text=some_text<and>some%0d%0achars\"", rpFromCgi.GetURL());
    }
}

Y_UNIT_TEST(TestRetPathFromBadCgi) {
    const TStringBuf BAD_CGI[] = {
        // no underscore in the string
        "http://yandex.com.tr/yandsearch?text=cats&lr=130",
        // signature is corrupted
        "http://yandex.com.tr/yandsearch?text=cats&lr=130_9d129157cd3fb9b1f1df3765def55d1",
        // url doesn't correspond to the signature
        "http://yandex.com.tr/yandsearch?text=dogs&lr=130_9d129157cd3fb9b1f1df3765def55d10",
    };
    for (size_t i = 0; i < Y_ARRAY_SIZE(BAD_CGI); ++i) {
        TCgiParameters cgi;
        cgi.InsertUnescaped("retpath", BAD_CGI[i]);
        UNIT_ASSERT_EXCEPTION(TReturnPath::FromCgi(cgi), TReturnPath::TInvalidRetPathException);
    }
}

Y_UNIT_TEST(TestHttpResponseSplittingDefense) {
    const TString REQUEST = "GET /yandsearch?text=some_text%0d%0aheader:%20value HTTP/1.0\r\n"
                           "Accept: */*, text/html, application/xhtml+xml\r\n"
                           "User-Agent: Mozilla/4.0 (compatible; MSIE 7.0b; Windows NT 6.0 ; .NET CLR 2.0.50215; SL Commerce Client v1.0; Tablet PC 2.0\r\n"
                           "Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r\n"
                           "Accept-Charset: windows-1251,utf-8;q=0.7,*;q=0.3\r\n"
                           "Host: yandex.com.tr\r\n"
                           "Proxy-Connection: Keep-Alive\r\n"
                           "Connection: Keep-Alive\r\n"
                           "Keep-Alive: 361\r\n"
                           "Pragma: no-cache\r\n"
                           "X-Forwarded-For-Y: 188.186.226.2\r\n"
                           "X-Source-Port-Y: 10769\r\n"
                           "X-Req-Id: 1354193658048134-768518413483003042\r\n"
                           "\r\n";

    TAutoPtr<TRequest> req = CreateRequest(REQUEST);
    UNIT_ASSERT_NO_EXCEPTION(TReturnPath::FromRequest(*req));

    TReturnPath rp = TReturnPath::FromRequest(*req);
    TCgiParameters cgi;
    rp.AddToCGI(cgi);
    UNIT_ASSERT_NO_EXCEPTION(TReturnPath::FromCgi(cgi));

    THttpResponse resp = THttpResponse(HTTP_FOUND).AddHeader("Location", TReturnPath::FromCgi(cgi).GetURL());

    TString response;
    TStringOutput out(response);
    out << resp;

    const char* EXPECTED_RESPONSE = "HTTP/1.1 302 Moved temporarily\r\n"
                                    "Location: http://yandex.com.tr/yandsearch?text=some_text%0d%0aheader:%20value\r\n"
                                    "\r\n";
    UNIT_ASSERT_STRINGS_EQUAL(response, EXPECTED_RESPONSE);
}

}
} /* namespace NAntiRobot */
