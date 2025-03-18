#include <library/cpp/testing/unittest/registar.h>

#include "fullreq_info.h"
#include "reloadable_data.h"

#include <antirobot/daemon_lib/ut/utils.h>

#include <util/folder/tempdir.h>

namespace NAntiRobot {

class TTestFullReqParams : public TTestBase {
public:
    void SetUp() override {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString("<Daemon>\n"
                                                       "FormulasDir = .\n"
                                                       "</Daemon>\n<Zone></Zone>");
    }
};

Y_UNIT_TEST_SUITE_IMPL(TestFullReq, TTestFullReqParams) {

TFullReqInfo MakeTFullReqInfo(const TString& request) {
    TReloadableData data;
    TStringInput stringInput{request};
    THttpInput input{&stringInput};
    TFullReqInfo req(input, "", "", data, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
    return req;
}

Y_UNIT_TEST(DecryptIcookie) {
    {
        const TString REQUEST_WO_ICOOKIE = "GET /search?text=cats HTTP/1.1\r\n"
                                          "Host: yandex.ru\r\n"
                                          "X-Forwarded-For-Y: 127.0.0.1\r\n"
                                          "X-Req-Id: 1398160169431228-5844437078103525702\r\n"
                                          "\r\n";

        TFullReqInfo req = MakeTFullReqInfo(REQUEST_WO_ICOOKIE);
        UNIT_ASSERT_EQUAL(req.Headers.Get("X-Yandex-ICookie"), "");
        UNIT_ASSERT(!req.HasAnyICookie);
        UNIT_ASSERT(!req.HasValidICookie);
    }
    {
        const TString REQUEST_DECRYPTED_ICOOKIE = "GET /search?text=cats HTTP/1.1\r\n"
                                                 "Host: yandex.ru\r\n"
                                                 "X-Forwarded-For-Y: 127.0.0.1\r\n"
                                                 "X-Req-Id: 1398160169431228-5844437078103525702\r\n"
                                                 "X-Yandex-ICookie: 1000017731498595999\r\n"
                                                 "\r\n";

        TFullReqInfo req = MakeTFullReqInfo(REQUEST_DECRYPTED_ICOOKIE);
        UNIT_ASSERT_UNEQUAL(req.Headers.Get("X-Yandex-ICookie"), "");
        UNIT_ASSERT(req.HasAnyICookie);
        UNIT_ASSERT(req.HasValidICookie);
    }
    {
        const TString REQUEST_ENCRYPTED_ICOOKIE = "GET /search?text=cats HTTP/1.1\r\n"
                                                 "Host: yandex.ru\r\n"
                                                 "X-Forwarded-For-Y: 127.0.0.1\r\n"
                                                 "X-Req-Id: 1398160169431228-5844437078103525702\r\n"
                                                 "Cookie: i=pQVrpmUcnXFe+l/LMCVxYEOz0tDThXLwqPFUvB7ysBJMlofNyWW7Q6+JdEbf0DQ+90GxkshMGejwjg4nTBjNe06jgHA=;\r\n"
                                                 "\r\n";

        TFullReqInfo req = MakeTFullReqInfo(REQUEST_ENCRYPTED_ICOOKIE);
        UNIT_ASSERT_UNEQUAL(req.Headers.Get("X-Yandex-ICookie"), "");
        UNIT_ASSERT(req.HasAnyICookie);
        UNIT_ASSERT(req.HasValidICookie);
    }
}

Y_UNIT_TEST(TestPrivilegedPartner) {
    // CAPTCHA-481
    TReloadableData data;
    {
        const TString PRIVILEGED_IP = "217.65.3.170";
        TStringInput si(PRIVILEGED_IP);
        TIpListProjId ips;
        ips.Load(si);
        data.PrivilegedIps.Set(std::move(ips));
    }

    const TString REQUEST = "GET /xmlsearch?text=test&showmecaptcha=yes HTTP/1.1\r\n"
                           "Host: yandex.ru\r\n"
                           "X-Forwarded-For-Y: 127.0.0.1\r\n"
                           "X-Real-Ip: 217.65.3.170\r\n"
                           "X-Req-Id: 1398160169431228-5844437078103525702\r\n"
                           "\r\n";

    TStringInput si(REQUEST);
    THttpInput hi(&si);

    TFullReqInfo req(hi, "", "", data, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
    UNIT_ASSERT(req.IsPartnerRequest());
    UNIT_ASSERT(req.UserAddr.IsPrivileged());
}


Y_UNIT_TEST(TestUaProxyIps) {
    const TString UA_PROXY_IP = "188.42.174.62";
    const TString IP = "46.118.212.4";
    TReloadableData data;
    {
        TStringInput si(UA_PROXY_IP);
        TIpListProjId ips;
        ips.Load(si);
        data.UaProxyIps.Set(std::move(ips));
    }

    const TString REQUEST = "GET /robots.txt HTTP/1.1\r\n"
                           "Host: yandex.ru\r\n"
                           "X-Forwarded-For-Y: " + UA_PROXY_IP + "\r\n"
                           "Shadow-X-Forwarded-For: " + IP + "\r\n"
                           "\r\n";

    TStringInput si(REQUEST);
    THttpInput hi(&si);

    TFullReqInfo req(hi, "", "", data, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
    UNIT_ASSERT(req.UserAddr.AsIp() == TAddr(IP).AsIp());
}

Y_UNIT_TEST(TestBadSign) {
    TReloadableData data;
    const TString REQUEST = "GET /showcaptcha?retpath=https%3A//yandex.ru%3A17245/se1arch%3Ftext%3Dvvv_f86976bd9f01fd6e7b67aa13e492df11&t=2/1619609249/09f8ffd63893d548e5a7a3499e2659bb&s=bf39cf7f2c96b85ce5d5a10dd1a150ff HTTP/1.1\r\n"
                           "Host: yandex.ru\r\n"
                           "X-Forwarded-For-Y: 46.118.212.4\r\n"
                           "\r\n";

    TStringInput si(REQUEST);
    THttpInput hi(&si);
    
    TFullReqInfo req(hi, "", "", data, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
    UNIT_ASSERT(req.HostType == HOST_OTHER);
}

Y_UNIT_TEST(TestHeaderPositions) {
    TReloadableData data;
    const TString REQUEST = "GET /showcaptcha?retpath=https%3A//yandex.ru%3A17245/se1arch%3Ftext%3Dvvv_f86976bd9f01fd6e7b67aa13e492df11&t=2/1619609249/09f8ffd63893d548e5a7a3499e2659bb&s=bf39cf7f2c96b85ce5d5a10dd1a150ff HTTP/1.1\r\n"
                           "Host: yandex.ru\r\n"
                           "X-Forwarded-For-Y: 146.24.129.47\r\n"
                           "Accept-Language: ru\r\n"
                           "\r\n";

    TStringInput si(REQUEST);
    THttpInput hi(&si);

    TFullReqInfo req(hi, "", "", data, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
    UNIT_ASSERT(req.HeaderPos[static_cast<size_t>(EHeaderOrderNames::Host)] == 1);
    UNIT_ASSERT(req.HeaderPos[static_cast<size_t>(EHeaderOrderNames::AcceptLanguage)] == 3);

}

Y_UNIT_TEST(TestChromeHodor) {
    {
        TReloadableData data;
        const TString REQUEST = "GET /showcaptcha?retpath=https%3A//yandex.ru%3A17245/se1arch%3Ftext%3Dvvv_f86976bd9f01fd6e7b67aa13e492df11&t=2/1619609249/09f8ffd63893d548e5a7a3499e2659bb&s=bf39cf7f2c96b85ce5d5a10dd1a150ff HTTP/1.1\r\n"
                                "Host: yandex.ru\r\n"
                                "X-Forwarded-For-Y: 146.24.129.47\r\n"
                                "Accept-Language: ru\r\n"
                                "\r\n";

        TStringInput si(REQUEST);
        THttpInput hi(&si);

        TFullReqInfo req(hi, "", "", data, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
        UNIT_ASSERT(!req.IsChrome());
    }

    {
        TReloadableData data;
        const TString REQUEST = "GET /showcaptcha?retpath=https%3A//yandex.ru%3A17245/se1arch%3Ftext%3Dvvv_f86976bd9f01fd6e7b67aa13e492df11&t=2/1619609249/09f8ffd63893d548e5a7a3499e2659bb&s=bf39cf7f2c96b85ce5d5a10dd1a150ff HTTP/1.1\r\n"
                                "Accept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, */*;q=0.1\r\n"
                                "Accept-Encoding: gzip, deflate\r\n"
                                "Accept-Language: ru\r\n"
                                "Host: yandex.ru\r\n"
                                "X-Forwarded-For-Y: 146.24.129.47\r\n"
                                "User-Agent: Opera/9.80 (Windows NT 5.1) Presto/2.12.388 Version/12.11\r\n"
                                "\r\n";

        TStringInput si(REQUEST);
        THttpInput hi(&si);

        TFullReqInfo req(hi, "", "", data, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
        UNIT_ASSERT(req.IsChrome());
    }

    {
        TReloadableData data;
        const TString REQUEST = "GET /showcaptcha?retpath=https%3A//yandex.ru%3A17245/se1arch%3Ftext%3Dvvv_f86976bd9f01fd6e7b67aa13e492df11&t=2/1619609249/09f8ffd63893d548e5a7a3499e2659bb&s=bf39cf7f2c96b85ce5d5a10dd1a150ff HTTP/1.1\r\n"
                                "Accept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, */*;q=0.1\r\n"
                                "Accept-Encoding: gzip, deflate\r\n"
                                "Host: yandex.ru\r\n"
                                "X-Forwarded-For-Y: 146.24.129.47\r\n"
                                "User-Agent: Opera/9.80 (Windows NT 5.1) Presto/2.12.388 Version/12.11\r\n"
                                "\r\n";

        TStringInput si(REQUEST);
        THttpInput hi(&si);

        TFullReqInfo req(hi, "", "", data, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
        UNIT_ASSERT(req.IsChrome());
    }

    {
        TReloadableData data;
        const TString REQUEST = "GET /showcaptcha?retpath=https%3A//yandex.ru%3A17245/se1arch%3Ftext%3Dvvv_f86976bd9f01fd6e7b67aa13e492df11&t=2/1619609249/09f8ffd63893d548e5a7a3499e2659bb&s=bf39cf7f2c96b85ce5d5a10dd1a150ff HTTP/1.1\r\n"
                                "Accept-Encoding: gzip, deflate\r\n"
                                "Accept-Language: ru\r\n"
                                "Host: yandex.ru\r\n"
                                "X-Forwarded-For-Y: 146.24.129.47\r\n"
                                "User-Agent: Opera/9.80 (Windows NT 5.1) Presto/2.12.388 Version/12.11\r\n"
                                "\r\n";

        TStringInput si(REQUEST);
        THttpInput hi(&si);

        TFullReqInfo req(hi, "", "", data, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
        UNIT_ASSERT(req.IsChrome());
    }

    {

        TReloadableData data;
        const TString REQUEST = "GET /showcaptcha?retpath=https%3A//yandex.ru%3A17245/se1arch%3Ftext%3Dvvv_f86976bd9f01fd6e7b67aa13e492df11&t=2/1619609249/09f8ffd63893d548e5a7a3499e2659bb&s=bf39cf7f2c96b85ce5d5a10dd1a150ff HTTP/1.1\r\n"
                                "Accept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, */*;q=0.1\r\n"
                                "Accept-Language: ru\r\n"
                                "Host: yandex.ru\r\n"
                                "X-Forwarded-For-Y: 146.24.129.47\r\n"
                                "User-Agent: Opera/9.80 (Windows NT 5.1) Presto/2.12.388 Version/12.11\r\n"
                                "Accept-Encoding: gzip, deflate\r\n"
                                "\r\n";

        TStringInput si(REQUEST);
        THttpInput hi(&si);

        TFullReqInfo req(hi, "", "", data, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
        UNIT_ASSERT(req.IsChrome());
    }

    {
        TReloadableData data;
        const TString REQUEST = "GET /showcaptcha?retpath=https%3A//yandex.ru%3A17245/se1arch%3Ftext%3Dvvv_f86976bd9f01fd6e7b67aa13e492df11&t=2/1619609249/09f8ffd63893d548e5a7a3499e2659bb&s=bf39cf7f2c96b85ce5d5a10dd1a150ff HTTP/1.1\r\n"
                                "Accept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, */*;q=0.1\r\n"
                                "Accept-Language: ru\r\n"
                                "Host: yandex.ru\r\n"
                                "X-Forwarded-For-Y: 146.24.129.47\r\n"
                                "User-Agent: Opera/9.80 (Windows NT 5.1) Presto/2.12.388 Version/12.11\r\n"
                                "\r\n";

        TStringInput si(REQUEST);
        THttpInput hi(&si);

        TFullReqInfo req(hi, "", "", data, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
        UNIT_ASSERT(req.IsChrome());
    }

    {
        TReloadableData data;
        const TString REQUEST = "GET /showcaptcha?retpath=https%3A//yandex.ru%3A17245/se1arch%3Ftext%3Dvvv_f86976bd9f01fd6e7b67aa13e492df11&t=2/1619609249/09f8ffd63893d548e5a7a3499e2659bb&s=bf39cf7f2c96b85ce5d5a10dd1a150ff HTTP/1.1\r\n"
                                "Accept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, */*;q=0.1\r\n"
                                "Host: yandex.ru\r\n"
                                "X-Forwarded-For-Y: 146.24.129.47\r\n"
                                "User-Agent: Opera/9.80 (Windows NT 5.1) Presto/2.12.388 Version/12.11\r\n"
                                "\r\n";

        TStringInput si(REQUEST);
        THttpInput hi(&si);

        TFullReqInfo req(hi, "", "", data, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
        UNIT_ASSERT(!req.IsChrome());
    }
}

Y_UNIT_TEST(EmptySenderAddr) {
    const TString REQUEST_WO_ADDRESS =   "GET /robots.txt HTTP/1.1\r\n"
                                        "Host: news.yandex.ru\r\n"
                                        "\r\n";
    const TString REQUEST_WITH_ADDRESS = "GET /robots.txt HTTP/1.1\r\n"
                                        "Host: news.yandex.ru\r\n"
                                        "X-Forwarded-For-Y: 127.0.0.1\r\n"
                                        "\r\n";
    UNIT_ASSERT_EXCEPTION(CreateDummyParsedRequest(REQUEST_WO_ADDRESS), TFullReqInfo::TUidCreationFailure);
    UNIT_ASSERT_NO_EXCEPTION(CreateDummyParsedRequest(REQUEST_WITH_ADDRESS));
}

bool GetTrusted(const TString& request, const TReloadableData& data) {
    TStringInput si(request);
    THttpInput hi(&si);

    TFullReqInfo req(hi, "", "", data, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
    return req.TrustedUser;
}

Y_UNIT_TEST(TrustedUser) {
    TReloadableData data;
    {
        TTrustedUsers users({1000017731498595997});
        data.TrustedUsers.Set(std::move(users));
    }

    const TString REQUEST_ICOOKIE = "GET /search?text=test&showmecaptcha=yes HTTP/1.1\r\n"
                           "Host: yandex.ru\r\n"
                           "X-Forwarded-For-Y: 127.0.0.1\r\n"
                           "X-Req-Id: 1398160169431228-5844437078103525702\r\n"
                           "X-Yandex-ICookie: 1000017731498595997\r\n"
                           "\r\n";

    const TString REQUEST_ICOOKIE_NOT_TRUSTED = "GET /search?text=test&showmecaptcha=yes HTTP/1.1\r\n"
                           "Host: yandex.ru\r\n"
                           "X-Forwarded-For-Y: 127.0.0.1\r\n"
                           "X-Req-Id: 1398160169431228-5844437078103525702\r\n"
                           "X-Yandex-ICookie: 1000017731498595999\r\n"
                           "\r\n";

    const TString REQUEST_YUID = "GET /search?text=test&showmecaptcha=yes HTTP/1.1\r\n"
                           "Host: yandex.ru\r\n"
                           "X-Forwarded-For-Y: 127.0.0.1\r\n"
                           "X-Req-Id: 1398160169431228-5844437078103525702\r\n"
                           "Cookie: yandexuid=1000017731498595997;"
                           "\r\n";

    // ICookie have move priority for TrustedUser
    const TString REQUEST_ICOOKIE_YUID = "GET /search?text=test&showmecaptcha=yes HTTP/1.1\r\n"
                           "Host: yandex.ru\r\n"
                           "X-Forwarded-For-Y: 127.0.0.1\r\n"
                           "X-Req-Id: 1398160169431228-5844437078103525702\r\n"
                           "X-Yandex-ICookie: 1000017731498595999\r\n"
                           "Cookie: yandexuid=1000017731498595997;"
                           "\r\n";

    UNIT_ASSERT(GetTrusted(REQUEST_ICOOKIE, data));
    UNIT_ASSERT(!GetTrusted(REQUEST_ICOOKIE_NOT_TRUSTED, data));
    UNIT_ASSERT(GetTrusted(REQUEST_YUID, data));
    UNIT_ASSERT(!GetTrusted(REQUEST_ICOOKIE_YUID, data));
}

TString GetUuid(const TString& request) {
    TStringInput si(request);
    THttpInput hi(&si);
    TReloadableData data;

    TFullReqInfo req(hi, "", "", data, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
    return TString(req.Uuid);
}

Y_UNIT_TEST(Uuid) {
    const TString REQUEST_WITHOUT_UUID = "GET /search?text=test&showmecaptcha=yes HTTP/1.1\r\n"
                           "Host: yandex.ru\r\n"
                           "X-Forwarded-For-Y: 127.0.0.1\r\n"
                           "X-Req-Id: 1398160169431228-5844437078103525702\r\n"
                           "X-Yandex-ICookie: 1000017731498595999\r\n"
                           "Cookie: yandexuid=1000017731498595997;"
                           "\r\n";

    const TString REQUEST_VALID_UUID_FROM_HEADERS = "GET /search?text=test&showmecaptcha=yes HTTP/1.1\r\n"
                           "Host: yandex.ru\r\n"
                           "X-Forwarded-For-Y: 127.0.0.1\r\n"
                           "X-Req-Id: 1398160169431228-5844437078103525702\r\n"
                           "X-Yandex-ICookie: 1000017731498595999\r\n"
                           "Cookie: yandexuid=1000017731498595997;\r\n"
                           "X-Device-UUID: 012345678901234567890123456789AB\r\n"
                           "\r\n";

    const TString REQUEST_INVALID_UUID_FROM_HEADERS = "GET /search?text=test&showmecaptcha=yes HTTP/1.1\r\n"
                           "Host: yandex.ru\r\n"
                           "X-Forwarded-For-Y: 127.0.0.1\r\n"
                           "X-Req-Id: 1398160169431228-5844437078103525702\r\n"
                           "X-Yandex-ICookie: 1000017731498595999\r\n"
                           "Cookie: yandexuid=1000017731498595997;\r\n"
                           "X-Device-UUID: 012345678901234567890123456789AZ\r\n"
                           "\r\n";

    const TString REQUEST_VALID_UUID_FROM_HEADERS2 = "GET /search?text=test&showmecaptcha=yes HTTP/1.1\r\n"
                           "Host: yandex.ru\r\n"
                           "X-Forwarded-For-Y: 127.0.0.1\r\n"
                           "X-Req-Id: 1398160169431228-5844437078103525702\r\n"
                           "X-Yandex-ICookie: 1000017731498595999\r\n"
                           "Cookie: yandexuid=1000017731498595997;\r\n"
                           "UUID: 012345678901234567890123456789Ac\r\n"
                           "\r\n";

    const TString REQUEST_VALID_UUID_FROM_HEADERS3 = "GET /search?text=test&showmecaptcha=yes HTTP/1.1\r\n"
                           "Host: yandex.ru\r\n"
                           "X-Forwarded-For-Y: 127.0.0.1\r\n"
                           "X-Req-Id: 1398160169431228-5844437078103525702\r\n"
                           "X-Yandex-ICookie: 1000017731498595999\r\n"
                           "Cookie: yandexuid=1000017731498595997;\r\n"
                           "UUID: 012345678901234567890123456789Ac\r\n"
                           "\r\n";

    const TString REQUEST_VALID_UUID_FROM_HEADERS4 = "GET /search?text=test&showmecaptcha=yes&uuid=012345678901234567890123456789AE HTTP/1.1\r\n"
                           "Host: yandex.ru\r\n"
                           "X-Forwarded-For-Y: 127.0.0.1\r\n"
                           "X-Req-Id: 1398160169431228-5844437078103525702\r\n"
                           "X-Yandex-ICookie: 1000017731498595999\r\n"
                           "Cookie: yandexuid=1000017731498595997;\r\n"
                           "\r\n";

    UNIT_ASSERT(GetUuid(REQUEST_WITHOUT_UUID).empty());
    UNIT_ASSERT(GetUuid(REQUEST_VALID_UUID_FROM_HEADERS) == "012345678901234567890123456789AB");
    UNIT_ASSERT(GetUuid(REQUEST_INVALID_UUID_FROM_HEADERS) == "012345678901234567890123456789AZ");
    UNIT_ASSERT(GetUuid(REQUEST_VALID_UUID_FROM_HEADERS2) == "012345678901234567890123456789Ac");
    UNIT_ASSERT(GetUuid(REQUEST_VALID_UUID_FROM_HEADERS3) == "012345678901234567890123456789Ac");
    UNIT_ASSERT(GetUuid(REQUEST_VALID_UUID_FROM_HEADERS4) == "012345678901234567890123456789AE");
}

}

} // namespace NAntiRobot
