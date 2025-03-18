#include "fullreq_info.h"
#include "req_types.h"
#include "request_params.h"

#include <antirobot/daemon_lib/ut/utils.h>

#include <antirobot/lib/ip_list.h>
#include <antirobot/lib/keyring.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>
#include <util/network/address.h>
#include <util/stream/str.h>
#include <util/string/join.h>
#include <util/string/printf.h>
#include <library/cpp/string_utils/quote/quote.h>

namespace NAntiRobot {

class TTestRequestParams : public TTestBase {
public:

    TString Name() const noexcept override {
        return "TTestRequestParams";
    }

    void SetUp() override {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString("<Daemon>\n"
                                                       "AuthorizeByFuid = 1\n"
                                                       "AuthorizeByICookie = 1\n"
                                                       "AuthorizeByLCookie = 1\n"
                                                       "CaptchaApiHost = ::\n"
                                                       "FormulasDir = .\n"
                                                       "CbbApiHost = ::\n"
                                                       "</Daemon>\n"
                                                       "<Zone></Zone>");

        TJsonConfigGenerator jsonConf;
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig.LoadFromString(jsonConf.Create(), GetJsonServiceIdentifierStr());
        const_cast<TAntirobotDaemonConfig::TZoneConf&>(ANTIROBOT_DAEMON_CONFIG_MUTABLE.ConfByTld("ru")).PartnerCaptchaType = true;
        const TString testKey = "102c46d700bed5c69ed20b7473886468";
        TStringInput keys(testKey);
        TKeyRing::SetInstance(TKeyRing(keys));
    }
};

Y_UNIT_TEST_SUITE_IMPL(TestRequestParams, TTestRequestParams) {

const TRequestClassifier& GetClassifier() {
    static TRequestClassifier classifier = CreateClassifierForTests();
    return classifier;
}

const TRequestGroupClassifier& GetRequestGroupClassifier() {
    static TRequestGroupClassifier classifier = CreateRequestGroupClassifierForTests();
    return classifier;
}

TString RemoveLrFromUrl(TString request) {
    const TStringBuf LR = "&lr=";
    size_t beg = request.find(LR);
    if (beg == TString::npos) {
        return request;
    }
    size_t end = beg + LR.length();
    while (end < request.length() && isdigit(request[end])) {
        ++end;
    }
    return request.erase(beg, end - beg);
}

bool IsCookiesLine(const TString& line) {
    return line.StartsWith("Cookie");
}

TString RemoveLrFromCookiesLine(TString line) {
    const TStringBuf YANDEX_SEARCH_PREFS = "YX_SEARCHPREFS=";
    const TStringBuf LR = "LR%3A";

    size_t searchPrefsPos = line.find(YANDEX_SEARCH_PREFS);
    if (searchPrefsPos == TString::npos) {
        return line;
    }
    size_t lrPos = line.find(LR, searchPrefsPos + YANDEX_SEARCH_PREFS.length());
    if (lrPos == TString::npos) {
        return line;
    }
    size_t lrValuePos = lrPos + LR.length();
    while (lrValuePos < line.length() && isdigit(line[lrValuePos])) {
        ++lrValuePos;
    }
    return line.erase(lrPos, lrValuePos - lrPos);
}

TString RemoveLrFromCookies(const TString& request) {
    TString cur;
    TString result;

    for (TStringInput in(request); in.ReadLine(cur); ) {
        if (IsCookiesLine(cur)) {
            cur = RemoveLrFromCookiesLine(cur);
        }
        result += cur + "\r\n";
    }
    return result + "\r\n";
}

/// The test checks that requests with and without "lr" are parsed identically
void TestCouple(const TString& requestWithLr, const TString& requestWithoutLr) {
#define TEST_EQUAL(a, b) \
{ \
TString msg; \
TStringOutput out(msg); \
out << #a << " == " << (a) << ", " << #b << " == " << (b) << Endl \
    << "for requests:" << Endl << requestWithLr << requestWithoutLr; \
UNIT_ASSERT_EQUAL_C(a, b, msg); \
}
    TAutoPtr<TRequest> withLr = CreateDummyParsedRequest(requestWithLr, GetClassifier());
    TAutoPtr<TRequest> withoutLr = CreateDummyParsedRequest(requestWithoutLr, GetClassifier());

    TEST_EQUAL(withLr->ReqType, withoutLr->ReqType);
    TEST_EQUAL(withLr->Host, withoutLr->Host)
    TEST_EQUAL((int)withLr->ClientType, (int)withoutLr->ClientType)
    TEST_EQUAL(withLr->Uid, withoutLr->Uid);
    TEST_EQUAL(withLr->UserAddr, withoutLr->UserAddr);
    TEST_EQUAL(withLr->SpravkaAddr, withoutLr->SpravkaAddr);
    TEST_EQUAL(withLr->SpravkaTime, withoutLr->SpravkaTime);
    TEST_EQUAL(withLr->HasValidFuid, withoutLr->HasValidFuid);
    TEST_EQUAL(withLr->HasValidLCookie, withoutLr->HasValidLCookie);
    TEST_EQUAL(withLr->HasValidSpravka, withoutLr->HasValidSpravka);
    TEST_EQUAL(withLr->HostType, withoutLr->HostType)
    TEST_EQUAL((int)withLr->CaptchaReqType, (int)withoutLr->CaptchaReqType)
    TEST_EQUAL(withLr->YandexUid, withoutLr->YandexUid)
    TEST_EQUAL(withLr->UserAddr.GeoRegion(), withoutLr->UserAddr.GeoRegion())
    TEST_EQUAL(withLr->IsSearch, withoutLr->IsSearch)
    TEST_EQUAL(withLr->InitiallyWasXmlsearch, withoutLr->InitiallyWasXmlsearch)
    TEST_EQUAL(withLr->ForceShowCaptcha, withoutLr->ForceShowCaptcha)
}

Y_UNIT_TEST(TestLr) {
    // Requests with and without lr= should be parsed in the same way

    // All the requests in the TEST_REQUESTS contain "lr=" in URL and "LR:" in
    // YX_SEARCHPREFS cookie.
    static const char* TEST_REQUESTS[] = {
        "GET /yandsearch?text=%22%D0%A1%D0%BE%D0%BE%D0%B1%D1%89%D0%B5%D0%BD%D0%B8%D1%8F+%D0%B7%D0%B0+%D0%B4%D0%B5%D0%BD%D1%8C%22+%22%D0%AD%D1%82%D0%B0+%D1%81%D1%82%D1%80%D0%B0%D0%BD%D0%B8%D1%86%D0%B0+%D0%B1%D1%8B%D0%BB%D0%B0+%D0%BF%D0%BE%D1%81%D0%B5%D1%89%D0%B5%D0%BD%D0%B0%22+%D0%BA%D1%80%D0%BE%D1%85%D0%B0&lr=130 HTTP/1.0\r\n"
        "Accept: */*, text/html, application/xhtml+xml\r\n"
        "User-Agent: Mozilla/4.0 (compatible; MSIE 7.0b; Windows NT 6.0 ; .NET CLR 2.0.50215; SL Commerce Client v1.0; Tablet PC 2.0\r\n"
        "Referer: http://yandex.ru/yandsearch?text=%22%D0%A1%D0%BE%D0%BE%D0%B1%D1%89%D0%B5%D0%BD%D0%B8%D1%8F+%D0%B7%D0%B0+%D0%B4%D0%B5%D0%BD%D1%8C%22+%22%D0%AD%D1%82%D0%B0+%D1%81%D1%82%D1%80%D0%B0%D0%BD%D0%B8%D1%86%D0%B0+%D0%B1%D1%8B%D0%BB%D0%B0+%D0%BF%D0%BE%D1%81%D0%B5%D1%89%D0%B5%D0%BD%D0%B0%22+%D0%BA%D1%80%D0%BE%D1%85%D0%B0&lr=130\r\n"
        "Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r\n"
        "Accept-Charset: windows-1251,utf-8;q=0.7,*;q=0.3\r\n"
        "Host: yandex.ru\r\n"
        "Proxy-Connection: Keep-Alive\r\n"
        "Connection: Keep-Alive\r\n"
        "Keep-Alive: 361\r\n"
        "Pragma: no-cache\r\n"
        "Cookie: YX_SEARCHPREFS=LR%3A130\r\n"
        "X-Forwarded-For-Y: 188.186.226.2\r\n"
        "X-Source-Port-Y: 10769\r\n"
        "X-Start-Time: 1354193658048134\r\n"
        "X-Req-Id: 1354193658048134-768518413483003042\r\n"
        "\r\n",

        "GET /yandsearch?p=3&text=nude+%22Dany+Carrel%22&clid=47639&lr=48 HTTP/1.1\r\n"
        "User-Agent: Opera/9.80 (Windows NT 5.1) Presto/2.12.388 Version/12.11\r\n"
        "Host: yandex.ru\r\n"
        "Accept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, image/webp, image/jpeg, image/gif, image/x-xbitmap, */*;q=0.1\r\n"
        "Accept-Language: ru-RU,ru;q=0.9,en;q=0.8\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Referer: http://yandex.ru/yandsearch?p=2&text=nude+%22Dany+Carrel%22&clid=47639&lr=48\r\n"
        "Cookie: yandexuid=8865387721327286324; fuid01=4d0c80113182266b.RPwlpOqSjm0jzeuqe7_WYTpHGDYIqEXWG54kVVL09dljmxkDj_Oh2W46eiJ5uMYe9A9_5dgbVFeXaOaUZyjnxFVjHth5Bitrx8XlOVvk6Iy8n2lzNl6tybUZlXuZ7nKu; my=YygCMIDVNgEAAA==; L=YmBVf2MHe3wHUGV9c3dcekJlW3dXcmNrJnILIQIIPh0FVHoWAwxZKx8fIAYGYFQEVQAeOjxVVCBPaxc2XwM6EQ==.1353132907.9541.214082.9e18ccbbb6ca59bc5cf825d8fb5398ec; yabs-frequency=/4/0W0W03QDfL3ClQDG/Je40DPWGGKqd0qQ046UhsG96W11k/; yp=1354450287.clh.47639; aw=2_; YX_SEARCHPREFS=LR%3A48\r\n"
        "Connection: Keep-Alive\r\n"
        "X-Forwarded-For-Y: 188.186.202.78\r\n"
        "X-Source-Port-Y: 54542\r\n"
        "X-Start-Time: 1354193658054839\r\n"
        "X-Req-Id: 1354193658054839-6563412409256195394\r\n"
        "\r\n",

        "GET /yandsearch?text=%D1%82%D1%80%D0%B8+%D0%B1%D0%BE%D0%B3%D0%B0%D1%82%D1%8B%D1%80%D1%8F+%D0%B8+%D1%88%D0%B0%D0%BC%D0%B0%D1%85%D0%B0%D0%BD%D1%81%D0%BA%D0%B0%D1%8F+%D1%86%D0%B0%D1%80%D0%B8%D1%86%D0%B0+%D1%81%D0%BC%D0%BE%D1%82%D1%80%D0%B5%D1%82%D1%8C+%D0%BE%D0%BD%D0%BB%D0%B0%D0%B9%D0%BD&lr=213 HTTP/1.1\r\n"
        "Accept: image/gif, image/jpeg, image/pjpeg, image/pjpeg, application/x-shockwave-flash, application/msword, application/vnd.ms-excel, application/x-ms-application, application/x-ms-xbap, application/vnd.ms-xpsdocument, application/xaml+xml, */*\r\n"
        "Referer: http://www.yandex.ru/\r\n"
        "Accept-Language: ru\r\n"
        "User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0; GTB7.3; .NET CLR 1.1.4322; .NET CLR 2.0.50727; .NET CLR 3.0.4506.2152; .NET CLR 3.5.30729)\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Host: yandex.ru\r\n"
        "Connection: Keep-Alive\r\n"
        "Cookie: yandexuid=2274023871349874988; YX_SEARCHPREFS=LR%3A213; yp=1354452722.clh.1821926; yabs-frequency=/4/2G0101b7jr01sAbG/Ko05DPmG8R8D1JMS466k1W0Nb137ePW4BOWGPpoT12s846S0/; fuid01=4f3575c200060fca.LSmjbZwH-9Qvw95FRDUBkw1rvKbO1L6I-2n2OvBlFQOvfVk3QNal9UsvPdroofxIiMJ4vA9TwsC9tKiHbzq4U4XMzFTXMm070M_EDgyj2cG2NvBfSQ7KdGnPpZDSvVEK; aw=2_; my=YygBgNUA; ys=bar.IE.6.5.0.1829%23gpauto.55_755768%3A37_617672%3A100000%3A3%3A1354193435\r\n"
        "X-Forwarded-For-Y: 37.204.40.154\r\n"
        "X-Source-Port-Y: 36356\r\n"
        "X-Start-Time: 1354193658053805\r\n"
        "X-Req-Id: 1354193658053805-2835122962823032557\r\n"
        "\r\n",

        "GET /sitesearch?p=1&text=%D0%BA%D0%B0%D0%BA%20%D0%B8%D1%81%D0%BF%D0%B5%D1%87%D1%8C%20%20%D0%BD%D0%BE%D0%B2%D0%BE%D0%B3%D0%BE%D0%B4%D0%BD%D0%B8%D0%B9%20%D0%B4%D0%BE%D0%BC%D0%B8%D0%BA&web=0&searchid=231960&outenc=windows-1251&frame=1&topdoc=http%3A%2F%2Fwww.kuharka.ru%2Fyasearch.html%3Ftext%3D%25EA%25E0%25EA%2B%25E8%25F1%25EF%25E5%25F7%25FC%2B%2B%25ED%25EE%25E2%25EE%25E3%25EE%25E4%25ED%25E8%25E9%2B%25E4%25EE%25EC%25E8%25EA%2B%26searchid%3D231960&refer_site=www.kuharka.ru&lr=143 HTTP/1.1\r\n"
        "Host: yandex.ru\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 6.1; rv:16.0) Gecko/20100101 Firefox/16.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.5,en;q=0.3\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Connection: keep-alive\r\n"
        "Referer: http://yandex.ru/sitesearch?text=%EA%E0%EA+%E8%F1%EF%E5%F7%FC++%ED%EE%E2%EE%E3%EE%E4%ED%E8%E9+%E4%EE%EC%E8%EA+&l10n=ru&web=0&searchid=231960&outenc=windows-1251&frame=1&topdoc=http%3A%2F%2Fwww.kuharka.ru%2Fyasearch.html%3Ftext%3D%25EA%25E0%25EA%2B%25E8%25F1%25EF%25E5%25F7%25FC%2B%2B%25ED%25EE%25E2%25EE%25E3%25EE%25E4%25ED%25E8%25E9%2B%25E4%25EE%25EC%25E8%25EA%2B%26searchid%3D231960\r\n"
        "Cookie: yandexuid=1200942621346597638; fuid01=50634a1d708bbd3d.C-RxmL8O4cQtMW-LRafCTSNAzaUfdF7INvbZCeQSFiWGMfUvFn-ZqsjRSRZT1gvo4LGsJWoli4kzXWG9--3p2dTQ9sOYoL7lvT-EMAgnQUVBaIxlNgSMv4bJTIplnIgc; YX_SEARCHPREFS=LR%3A143\r\n"
        "X-Forwarded-For-Y: 94.248.54.10\r\n"
        "X-Source-Port-Y: 16347\r\n"
        "X-Start-Time: 1354193670051783\r\n"
        "X-Req-Id: 1354193670051783-14483858061651647861\r\n"
        "\r\n",

        "GET /sitesearch?text=%D0%BF%D0%BB%D0%B8%D1%82%D0%B0+%D0%92%D0%95%D0%9A%D0%9E&searchid=125949&lr=55&web=0 HTTP/1.1\r\n"
        "Host: yandex.ru\r\n"
        "Connection: keep-alive\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.91 Safari/537.11\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Referer: http://yandex.ru/sitesearch?text=%D0%BF%D1%80%D0%BE%D0%B4%D0%B0%D0%BC+%D1%85%D0%BE%D0%BB%D0%BE%D0%B4%D0%B8%D0%BB%D1%8C%D0%BD%D0%B8%D0%BA&searchid=125949&lr=55&web=0\r\n"
        "Accept-Encoding: gzip,deflate,sdch\r\n"
        "Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r\n"
        "Accept-Charset: windows-1251,utf-8;q=0.7,*;q=0.3\r\n"
        "Cookie: yandexuid=2378324611353269084; fuid01=50aa86cf471a055a.QtmitHuInuKiV2h2qKN11TutuNtyz_dqwF5eIN2WGrExi265R1pW29Fuv9c78QE30AmI45hkhgrmKzrbnxOMIcwOuUhCvF99G2aM2nmjXfHYzmR0zjlOETrhn3DiuJep; YX_SEARCHPREFS=LR%3A55; aw=2_; yabs-frequency=/4/0G0006fNjr000000/\r\n"
        "X-Forwarded-For-Y: 188.186.27.121\r\n"
        "X-Source-Port-Y: 49889\r\n"
        "X-Start-Time: 1354193674432564\r\n"
        "X-Req-Id: 1354193674432564-15788024343763695159\r\n"
        "\r\n",

        "GET /sitesearch?text=%D0%BF%D0%BB%D0%B8%D1%82%D0%B0+BEKO&searchid=125949&lr=55&web=0 HTTP/1.1\r\n"
        "Host: yandex.ru\r\n"
        "Connection: keep-alive\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.91 Safari/537.11\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Referer: http://yandex.ru/sitesearch?text=%D0%BF%D0%BB%D0%B8%D1%82%D0%B0+%D0%92%D0%95%D0%9A%D0%9E&searchid=125949&lr=55&web=0\r\n"
        "Accept-Encoding: gzip,deflate,sdch\r\n"
        "Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r\n"
        "Accept-Charset: windows-1251,utf-8;q=0.7,*;q=0.3\r\n"
        "Cookie: yandexuid=2378324611353269084; YX_SEARCHPREFS=LR%3A55; fuid01=50aa86cf471a055a.QtmitHuInuKiV2h2qKN11TutuNtyz_dqwF5eIN2WGrExi265R1pW29Fuv9c78QE30AmI45hkhgrmKzrbnxOMIcwOuUhCvF99G2aM2nmjXfHYzmR0zjlOETrhn3DiuJep; aw=2_; yabs-frequency=/4/0W0000jRjr000000/\r\n"
        "X-Forwarded-For-Y: 188.186.27.121\r\n"
        "X-Source-Port-Y: 49889\r\n"
        "X-Start-Time: 1354193683189796\r\n"
        "X-Req-Id: 1354193683189796-695897935732766877\r\n"
        "\r\n",
    };

    for (size_t i = 0; i < Y_ARRAY_SIZE(TEST_REQUESTS); ++i) {
        const TString noLrAtAll = RemoveLrFromCookies(RemoveLrFromUrl(TEST_REQUESTS[i]));
        TestCouple(RemoveLrFromCookies(TEST_REQUESTS[i]), noLrAtAll);
        TestCouple(RemoveLrFromUrl(TEST_REQUESTS[i]), noLrAtAll);
    }
}

Y_UNIT_TEST(TestReqTypeAndHostType) {
    struct TTestCase {
        EHostType HostType;
        EReqType ReqType;
        EClientType ClientType;
        TString ReqGroupName;
        TString Request;
    };

    // Набор тестов в request_params_ut.inc не является исчерпывающим, в нём нет тестов
    // для некоторых типов запрсов. Они должны быть добавлены со временем.
    const TTestCase TEST_CASES[] = {
#include "request_params_ut.inc"
    };

    const auto& groupClassifier = GetRequestGroupClassifier();

    for (size_t i = 0; i < Y_ARRAY_SIZE(TEST_CASES); ++i) {
        THolder<TRequest> req(CreateDummyParsedRequest(TEST_CASES[i].Request, GetClassifier(), groupClassifier));
        UNIT_ASSERT_VALUES_EQUAL_C(req->ReqType, TEST_CASES[i].ReqType, TEST_CASES[i].Request);
        UNIT_ASSERT_VALUES_EQUAL_C(req->HostType, TEST_CASES[i].HostType, TEST_CASES[i].Request);
        UNIT_ASSERT_VALUES_EQUAL_C(req->ClientType, TEST_CASES[i].ClientType, TEST_CASES[i].Request);
        UNIT_ASSERT_VALUES_EQUAL_C(groupClassifier.GroupName(req->HostType, req->ReqGroup), TEST_CASES[i].ReqGroupName, TEST_CASES[i].Request);
    }
}

Y_UNIT_TEST(TestNoSpravka) {
    const TString REQUEST =  "GET /yandsearch?text=%D1%82%D1%80%D1%83%D0%B1%D0%B8%D0%BD%D1%81%D0%BA%D0%B0%D1%8F+%D0%B1%D HTTP/1.1\r\n"
                            "Host: yandex.ru\r\n"
                            "Connection: keep-alive\r\n"
                            "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.64 Safari/5\r\n"
                            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
                            "Referer: http://yandex.ru/yandsearch?text=%D0%BE%D0%B4%D0%BD%D0%BE%D0%BA%D0%BB%D0%B0%D1%81%D1%81%D0%BD%D0%B8&clid=1\r\n"
                            "Accept-Encoding: gzip,deflate,sdch\r\n"
                            "Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r\n"
                            "Accept-Charset: windows-1251,utf-8;q=0.7,*;q=0.3\r\n"
                            "Cookie: yandexuid=8388098111348154493\r\n"
                            "X-Forwarded-For-Y: 46.44.44.50\r\n"
                            "X-Source-Port-Y: 49644\r\n"
                            "X-Start-Time: 1352889168969432\r\n"
                            "X-Req-Id: 1352889168969432-10545734124049569903\r\n"
                            "\r\n";

    TAutoPtr<TRequest> rp = CreateDummyParsedRequest(REQUEST, GetClassifier());

    UNIT_ASSERT_VALUES_EQUAL(rp->SpravkaTime, TInstant());
    UNIT_ASSERT_VALUES_EQUAL(rp->SpravkaAddr, rp->UserAddr);
    UNIT_ASSERT_VALUES_EQUAL(rp->Uid, TUid::FromAddr(rp->UserAddr));
}

Y_UNIT_TEST(TestWithSpravkaInCookies) {
    const TString REQUEST =  "GET /yandsearch?text=rere&clid=1955453 HTTP/1.1\r\n"
                            "Host: yandex.ru\r\n"
                            "Connection: keep-alive\r\n"
                            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
                            "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.22 (KHTML, like Gecko) Chrome/25.0.1364.172 YaBrowser/1.7.1364.15751 Safari/537.22\r\n"
                            "Referer: http://yandex.ru/showcaptcha?retpath=http%3A//yandex.ru/yandsearch%3Ftext%3Drere%26clid%3D1955453_7cea248953a98010ffdddf2a906e37cb&t=0/1372418184/9bc4334603f11663b360e58d18271210&status=failed&s=d424ec362ef6ab818f89d7e8743e1aae\r\n"
                            "Accept-Encoding: gzip,deflate,sdch\r\n"
                            "Accept-Language: ru,en;q=0.8\r\n"
                            "Accept-Charset: windows-1251,utf-8;q=0.7,*;q=0.3\r\n"
                            "Cookie: z=s:l:36.272:1372063598006; yandexuid=6115051091369394953; fuid01=519f51ac488e26dc.BLN1Qa-cDQqIxYw0LLFyjWFR9Sf0g1si9u96xzKgDFqFEEg6IkVntep0g9YnTXSncJ1ToA4BhGYLy0MJQ5mSMEIprI2DR4Wqcn8czDSGL6umT2ic7pW2jalNNj-r5Av6; yandex_login=ishfb; my=YzYBAQA=; L=YlZ2Wk9LRVlmRQxcA0t9XQVHDWtRbQ98RQYlOCsMLjcAQxQDSwMCMgMBVAc7dXkFLRFjP1A5HgxATjtaBgEeIA==.1372150369.9796.233182.45795f26a6e9527280094f6df769aaee; yp=1372578690.clh.1955451#1687510369.udn.aXNoZmI=#1685262857.yb.1_7_1364_13754:0:1369902857322; __utma=190882677.1194126437.1372329524.1372329524.1372329524.1; __utmz=190882677.1372329524.1.1.utmcsr=mail.yandex.ru|utmccn=(referral)|utmcmd=referral|utmcct=/neo2/; __utmv=190882677.|2=Account=Yes=1^3=Login=Yes=1; ys=translate.chrome.2.0.85; Session_id=2:1372406320.-79.5.108723764.8:1372150369437:1600957818:14.668:1.669:1.0.1.1.0.94913.5481.bd7c861cdbc643e1db7e003cd01d0091; yabs-frequency=/4/I0000CL8pL400000/czS85zmJF000/; _ym_visorc=b; spravka=dD0xMzczOTAzODIyO2k9MTI3LjAuMC4xO3U9MTM3MzkwMzgyMjIyNDM0MzA2MTtoPTZjZmYxN2JlM2E3MThiN2UzNDJmOTVlMDI2NzU2MjI0\r\n"
                            "X-Forwarded-For-Y: 46.44.44.50\r\n"
                            "X-Source-Port-Y: 49644\r\n"
                            "X-Req-Id: 1352889168969432-10545734124049569903\r\n"
                            "\r\n";

    TAutoPtr<TRequest> rp = CreateDummyParsedRequest(REQUEST, GetClassifier());
    UNIT_ASSERT_VALUES_EQUAL(rp->Uid.Ns, TUid::SPRAVKA);
}

Y_UNIT_TEST(TestWithSpravkaInCgi) {
    const TString REQUEST =  "GET /sitesearch?html=1&spravka=dD0xMzczOTAzODIyO2k9MTI3LjAuMC4xO3U9MTM3MzkwMzgyMjIyNDM0MzA2MTtoPTZjZmYxN2JlM2E3MThiN2UzNDJmOTVlMDI2NzU2MjI0&topdoc=http%3A%2F%2Fhtmlcss.cyberskunk.lori.yandex.ru%2Fhtmlcss-dev_dev.html%3Fsearchid%3D1775761%26text%3Deeeee%26web%3D0%26lol%3D1&encoding=windows-1251&language=ru&tld=ru&htmlcss=1.x&updatehash=false&searchid=1775761&clid=&text=eeeee&web=0&p=&surl=&constraintid=&date=&within=&from_day=&from_month=&from_year=&to_day=&to_month=&to_year= HTTP/1.1\r\n"
                            "Host: cyberskunk-d-dev.leon02.yandex.ru\r\n"
                            "Connection: keep-alive\r\n"
                            "Accept: */*\r\n"
                            "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.22 (KHTML, like Gecko) Chrome/25.0.1364.172 YaBrowser/1.7.1364.15751 Safari/537.22\r\n"
                            "Accept-Encoding: gzip,deflate,sdch\r\n"
                            "Accept-Language: ru,en;q=0.8\r\n"
                            "Accept-Charset: windows-1251,utf-8;q=0.7,*;q=0.3\r\n"
                            "Cookie: yandexuid=6115051091369394953; fuid01=519f51ac488e26dc.BLN1Qa-cDQqIxYw0LLFyjWFR9Sf0g1si9u96xzKgDFqFEEg6IkVntep0g9YnTXSncJ1ToA4BhGYLy0MJQ5mSMEIprI2DR4Wqcn8czDSGL6umT2ic7pW2jalNNj-r5Av6; spravka=dD0xMzcyMDYzNTk3O2k9OTUuMTA4LjE3My4xMjI7dT0xMzcyMDYzNTk3MzcwMTc0NDA5O2g9MTY3ZmExOGJlODVmMGU3YzBhMGVlMTBmMTAwZTgwZGQ=; yandex_login=ishfb; my=YzYBAQA=; L=YlZ2Wk9LRVlmRQxcA0t9XQVHDWtRbQ98RQYlOCsMLjcAQxQDSwMCMgMBVAc7dXkFLRFjP1A5HgxATjtaBgEeIA==.1372150369.9796.233182.45795f26a6e9527280094f6df769aaee; Session_id=2:1372150369.-79.5.108723764.8:1372150369437:1600957818:14.668:1.669:1.0.1.1.0.94771.6154.7fe0f14f14e62ce2323474cd83990955; ys=translate.chrome.2.0.85; yp=1372497160.clh.1955451#1687510369.udn.aXNoZmI=#1685262857.yb.1_7_1364_13754:0:1369902857322; yabs-frequency=/4/F00000Qqob400000/DuW55-0JOPlN21VS4pm0/\r\n"
                            "X-Forwarded-For-Y: 95.108.173.122\r\n"
                            "X-Source-Port-Y: 49657\r\n"
                            "X-Req-Id: 1370003815418635-12676326873088255706\r\n"
                            "\r\n";

    TAutoPtr<TRequest> rp = CreateDummyParsedRequest(REQUEST, GetClassifier());
    UNIT_ASSERT_VALUES_EQUAL(rp->Uid.Ns, TUid::SPRAVKA);
}

Y_UNIT_TEST(Identifications) {
    // XXX чтобы тест работал в SetUp() разрешается идентификация по тестируемым сущностям.

    const TString COMMON_REQUEST =  "GET /yandsearch?text=rere&clid=1955453 HTTP/1.1\r\n"
                            "Host: yandex.ru\r\n"
                            "Connection: keep-alive\r\n"
                            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
                            "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.22 (KHTML, like Gecko) Chrome/25.0.1364.172 YaBrowser/1.7.1364.15751 Safari/537.22\r\n"
                            "Referer: http://yandex.ru/showcaptcha?retpath=http%3A//yandex.ru/yandsearch%3Ftext%3Drere%26clid%3D1955453_7cea248953a98010ffdddf2a906e37cb&t=0/1372418184/9bc4334603f11663b360e58d18271210&status=failed&s=d424ec362ef6ab818f89d7e8743e1aae\r\n"
                            "Accept-Encoding: gzip,deflate,sdch\r\n"
                            "Accept-Language: ru,en;q=0.8\r\n"
                            "Accept-Charset: windows-1251,utf-8;q=0.7,*;q=0.3\r\n"
                            "X-Forwarded-For-Y: 46.44.44.50\r\n"
                            "X-Source-Port-Y: 49644\r\n"
                            "X-Start-Time: 1379912431064704\r\n"
                            "X-Req-Id: 1379912431064704-10545734124049569903";
    const TString COMMON_COOKIES_WITH_HEADER = "Cookie: z=s:l:36.272:1372063598006; yandexuid=6115051091369394953; yandex_login=ishfb; my=YzYBAQA=;  yp=1372578690.clh.1955451#1687510369.udn.aXNoZmI=#1685262857.yb.1_7_1364_13754:0:1369902857322; __utma=190882677.1194126437.1372329524.1372329524.1372329524.1; __utmz=190882677.1372329524.1.1.utmcsr=mail.yandex.ru|utmccn=(referral)|utmcmd=referral|utmcct=/neo2/; __utmv=190882677.|2=Account=Yes=1^3=Login=Yes=1; ys=translate.chrome.2.0.85; Session_id=2:1372406320.-79.5.108723764.8:1372150369437:1600957818:14.668:1.669:1.0.1.1.0.94913.5481.bd7c861cdbc643e1db7e003cd01d0091; yabs-frequency=/4/I0000CL8pL400000/czS85zmJF000/; _ym_visorc=b";
    const TString COOKIE_FUID = "fuid01=519f51ac488e26dc.BLN1Qa-cDQqIxYw0LLFyjWFR9Sf0g1si9u96xzKgDFqFEEg6IkVntep0g9YnTXSncJ1ToA4BhGYLy0MJQ5mSMEIprI2DR4Wqcn8czDSGL6umT2ic7pW2jalNNj-r5Av6";
    const TString COOKIE_L = "L=YlZ2Wk9LRVlmRQxcA0t9XQVHDWtRbQ98RQYlOCsMLjcAQxQDSwMCMgMBVAc7dXkFLRFjP1A5HgxATjtaBgEeIA==.1372150369.9796.233182.45795f26a6e9527280094f6df769aaee";
    const TString COOKIE_SPRAVKA = "spravka=dD0xMzczOTAzODIyO2k9MTI3LjAuMC4xO3U9MTM3MzkwMzgyMjIyNDM0MzA2MTtoPTZjZmYxN2JlM2E3MThiN2UzNDJmOTVlMDI2NzU2MjI0";
    const TString HEADER_I_COOKIE = "X-Yandex-ICookie: 6115051091369394953";

    const TStringBuf CRLF = "\r\n";

    {
        TAutoPtr<TRequest> rp = CreateDummyParsedRequest(Join(CRLF, COMMON_REQUEST, COMMON_COOKIES_WITH_HEADER, ""), GetClassifier());
        UNIT_ASSERT_VALUES_EQUAL(rp->Uid.Ns, TUid::IP);
    }
    {
        TAutoPtr<TRequest> rp = CreateDummyParsedRequest(Join(CRLF
                                                            , COMMON_REQUEST
                                                            , COMMON_COOKIES_WITH_HEADER
                                                            , HEADER_I_COOKIE
                                                            , ""
                                                        ), GetClassifier());
        UNIT_ASSERT_VALUES_EQUAL(rp->Uid.Ns, TUid::ICOOKIE);
    }
    /* XXX Тест на идентификацию по L-куке не добавлен, т.к. для него нужны ключи L-кук. */
    {
        TAutoPtr<TRequest> rp = CreateDummyParsedRequest(Join(CRLF
                                                            , COMMON_REQUEST
                                                            , Join("; "
                                                                    , COMMON_COOKIES_WITH_HEADER
                                                                    , COOKIE_FUID
                                                                    , COOKIE_L
                                                                    , COOKIE_SPRAVKA
                                                                )
                                                            , HEADER_I_COOKIE
                                                            , ""
                                                        ), GetClassifier());
        UNIT_ASSERT_VALUES_EQUAL(rp->Uid.Ns, TUid::SPRAVKA);
    }
}

Y_UNIT_TEST(TestWrongAntirobotServiceHeader) {
    const TString REQUEST = "GET /search.xml?hid=91503&page=2 HTTP/1.1\r\n"
                           "Host: antirobot.yandex.ru\r\n"
                           "X-Antirobot-Service-Y: abracadbra\r\n"  // we don't recognize this value
                           "X-Forwarded-For-Y: 202.47.117.167\r\n"
                           "X-Source-Port-Y: 24804\r\n"
                           "X-Req-Id: 1375954266972097-8626638277124794355\r\n"
                           "\r\n";
    UNIT_ASSERT_VALUES_EQUAL(CreateDummyParsedRequest(REQUEST, GetClassifier())->HostType, HOST_OTHER);
}

Y_UNIT_TEST(TestGetReqid) {
    {
        const TString REQUEST =
                            "GET /search.xml?hid=91503&page=2 HTTP/1.1\r\n"
                            "Host: yandex.ru\r\n"
                            "X-Forwarded-For-Y: 202.47.117.167\r\n"
                            "X-Source-Port-Y: 24804\r\n"
                            "X-Req-Id: 1375954266972097-8626638277124794355\r\n"
                            "\r\n";

        TAutoPtr<TRequest> rp = CreateDummyParsedRequest(REQUEST, GetClassifier());
        UNIT_ASSERT_VALUES_EQUAL(rp->ReqId, "1375954266972097-8626638277124794355");
        UNIT_ASSERT_VALUES_EQUAL(rp->ServiceReqId, "1375954266972097-8626638277124794355");
    }
    {
        // host is market, but no market reqid header
        const TString REQUEST =
                            "GET /search.xml?hid=91503&page=2 HTTP/1.1\r\n"
                            "Host: market.yandex.ru\r\n"
                            "X-Forwarded-For-Y: 202.47.117.167\r\n"
                            "X-Source-Port-Y: 24804\r\n"
                            "X-Req-Id: 1375954266972097-8626638277124794355\r\n"
                            "\r\n";

        TAutoPtr<TRequest> rp = CreateDummyParsedRequest(REQUEST, GetClassifier());
        UNIT_ASSERT_VALUES_EQUAL(rp->ReqId, "1375954266972097-8626638277124794355");
        UNIT_ASSERT_VALUES_EQUAL(rp->ServiceReqId, "1375954266972097-8626638277124794355");
    }
    {
        // host is market, market reqid is present
        const TString REQUEST =
                            "GET /search.xml?hid=91503&page=2 HTTP/1.1\r\n"
                            "Host: market.yandex.ru\r\n"
                            "X-Forwarded-For-Y: 202.47.117.167\r\n"
                            "X-Source-Port-Y: 24804\r\n"
                            "X-Req-Id: 1375954266972097-8626638277124794355\r\n"
                            "X-Market-Req-Id: 9865432109871123-ABCDE\r\n"
                            "\r\n";

        TAutoPtr<TRequest> rp = CreateDummyParsedRequest(REQUEST, GetClassifier());
        UNIT_ASSERT_VALUES_EQUAL(rp->ReqId, "1375954266972097-8626638277124794355");
        UNIT_ASSERT_VALUES_EQUAL(rp->ServiceReqId, "9865432109871123-ABCDE");
    }
    {
        // host is autoru reqid is present
        const TString REQUEST =
                            "GET /search.xml?hid=91503&page=2 HTTP/1.1\r\n"
                            "Host: antirobot\r\n"
                            "X-Antirobot-Service-Y: autoru\r\n"
                            "X-Forwarded-For-Y: 202.47.117.167\r\n"
                            "X-Source-Port-Y: 24804\r\n"
                            "X-Req-Id: 1375954266972097-8626638277124794355\r\n"
                            "X-Request-Id: 9865432109871123-ABCDE\r\n"
                            "\r\n";

        TAutoPtr<TRequest> rp = CreateDummyParsedRequest(REQUEST, GetClassifier());
        UNIT_ASSERT_VALUES_EQUAL(rp->ReqId, "1375954266972097-8626638277124794355");
        UNIT_ASSERT_VALUES_EQUAL(rp->ServiceReqId, "9865432109871123-ABCDE");
    }
}

Y_UNIT_TEST(PrintData) {
    const TString REQUEST = "GET /search.xml?hid=91503&page=2 HTTP/1.1\r\n"
                           "Host: antirobot.yandex.ru\r\n"
                           "X-Forwarded-For-Y: 202.47.117.167\r\n"
                           "X-Source-Port-Y: 24804\r\n"
                           "X-Req-Id: 1375954266972097-8626638277124794355\r\n";
    auto request = CreateDummyParsedRequest(REQUEST, GetClassifier());

    TString serializedRequest;
    TStringOutput so(serializedRequest);
    request->PrintData(so, /* forceMaskCookies := */ false);
    UNIT_ASSERT_STRINGS_EQUAL(REQUEST, serializedRequest);
}

Y_UNIT_TEST(NewSearchLocations) {
    const std::pair<const char*, const char*> testData[] = {
        {"/yandsearch?text=123", "/search?text=123"},
        {"/storeclick", "/search/storeclick"},
        {"/storerequest", "/search/storerequest"},
        {"/versions", "/search/versions"},
        {"/403.html", "/search/403.html"},
        {"/404.html", "/search/404.html"},
        {"/500.html", "/search/500.html"},
        {"/adresa-segmentator", "/search/adresa-segmentator"},
        {"/adult", "/search/adult"},
        {"/all-supported-params", "/search/all-supported-params"},
        {"/cgi-bin/hidereferer", "/search/cgi-bin/hidereferer"},
        {"/csp", "/search/csp"},
        {"/gateway", "/search/gateway"},
        {"/customize", "/search/customize"},
        {"/cycounter", "/search/cycounter"},
        {"/dzen", "/search/dzen"},
        {"/familysearch?text=123", "/search/family?text=123"},
        {"/geoanswer", "/search/geoanswer"},
        {"/hotwater", "/search/hotwater"},
        {"/search/inforequest", "/search/inforequest"},
        {"/largesearch?text=123", "/search/large?text=123"},
        {"/opensearch.xml", "/search/opensearch.xml"},
        {"/padsearch?text=123", "/search/pad?text=123"},
        {"/poll-stations", "/search/poll-stations"},
        {"/post-indexes", "/search/post-indexes"},
        {"/redir", "/search/redir"},
        {"/redir_warning", "/search/redir_warning"},
        {"/schoolsearch?text=123", "/search/school?text=123"},
        {"/sitesearch?text=123", "/search/site?text=123"},
        {"/sitesearch/opensearch.xml", "/search/site/opensearch.xml"},
        {"/smartsearch?text=123", "/search/smart/?text=123"},
        {"/msearch?text=123", "/search/smart/?text=123"},
        {"/msearchpart?text=123", "/search/smartpart?text=123"},
        {"/st/b-spec-adv/title.gif", "/search/st/b-spec-adv/title.gif"},
        {"/tail-log", "/search/tail-log"},
        {"/touchsearch?text=123", "/search/touch/?text=123"},
        {"/v", "/search/v"},
        {"/viewconfig", "/search/viewconfig"},
        {"/search/wizard", "/search/wizard"},
        {"/wpage", "/search/wpage"},
        {"/yandcache.js", "/search/yandcache.js"},
        {"/413.html", "/search/413.html"},
        {"/auto-regions", "/search/auto-regions"},
        {"/vl", "/search/vl"},
        {"/wmpreview", "/search/wmpreview"},
        {"/prefetch.txt", "/search/prefetch.txt"},
        {"/all-supported-flags", "/search/all-supported-flags"},
        {"/cgi-bin/all_noporno_xml.pl", "/search/cgi-bin/all_noporno_xml.pl"},
    };

    const char* requestPattern = "GET %s HTTP/1.1\r\n"
                                 "Host: yandex.ru\r\n"
                                 "X-Forwarded-For-Y: 1.2.3.4\r\n"
                                 "X-Req-Id: 1375954266972097-8626638277124794355\r\n"
                                 "\r\n";

    for (const auto& testCase : testData) {
        auto firstRequest = CreateDummyParsedRequest(Sprintf(requestPattern, testCase.first), GetClassifier());
        auto secondRequest = CreateDummyParsedRequest(Sprintf(requestPattern, testCase.second), GetClassifier());
        TString testString = TString::Join(testCase.first, " ", testCase.second);
        UNIT_ASSERT_VALUES_EQUAL_C(firstRequest->ReqType, secondRequest->ReqType, testString);
        UNIT_ASSERT_VALUES_EQUAL_C(firstRequest->HostType, secondRequest->HostType, testString);
    }
}

Y_UNIT_TEST(CaptchReqTypes) {
    const std::pair<const char*, ECaptchaReqType> testData[] = {
        {"/showcaptcha?retpath=some_url&t=some_token&s=sign", CAPTCHAREQ_SHOW},
        {"/checkcaptcha?key=some_key&retpath=some_url&rep=123", CAPTCHAREQ_CHECK},
        {"/xcheckcaptcha?key=some_key&retpath=some_url&rep=123", CAPTCHAREQ_CHECK},
        {"/checkcaptchajson?key=some_key&retpath=some_url&rep=123", CAPTCHAREQ_CHECK},
        {"/captchaimg?long_string", CAPTCHAREQ_IMAGE},
        {"/some_path?some_param=long_string", CAPTCHAREQ_NONE},
    };

    const char* requestPattern = "GET %s HTTP/1.1\r\n"
                                 "Host: yandex.ru\r\n"
                                 "X-Forwarded-For-Y: 1.2.3.4\r\n"
                                 "X-Real-Ip: 1.2.3.4\r\n"
                                 "X-Antirobot-Service-Y: web\r\n"
                                 "\r\n";
    for (const auto& testCase : testData) {
        auto req = CreateDummyParsedRequest(Sprintf(requestPattern, testCase.first), GetClassifier());
        UNIT_ASSERT_VALUES_EQUAL(req->CaptchaReqType, testCase.second);
    }
}

// https://st.yandex-team.ru/CAPTCHA-855
Y_UNIT_TEST(TestGeolocationIsNotSearch) {
    {
        const TString REQUEST = "GET /search/touch?text=416+149 HTTP/1.1\r\n"
                               "User-Agent: Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; Trident/6.0)\r\n"
                               "Accept: text/html, application/xhtml+xml, */*\r\n"
                               "Accept-Language: ru-RU\r\n"
                               "Host: yandex.ru\r\n"
                               "Connection: Keep-Alive\r\n"
                               "X-Forwarded-For-Y: 189.48.216.61\r\n"
                               "X-Source-Port-Y: 53465\r\n"
                               "X-Start-Time: 1379912431064704\r\n"
                               "X-Req-Id: 1379912431064704-15672417911069840626\r\n"
                               "\r\n";

         THolder<TRequest> req(CreateDummyParsedRequest(REQUEST, GetClassifier()));
         UNIT_ASSERT(req->IsSearch);
    }
    {
        const TString REQUEST = "GET /search/touch/adresa-geolocation?text=416+149 HTTP/1.1\r\n"
                               "User-Agent: Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; Trident/6.0)\r\n"
                               "Accept: text/html, application/xhtml+xml, */*\r\n"
                               "Accept-Language: ru-RU\r\n"
                               "Host: yandex.ru\r\n"
                               "Connection: Keep-Alive\r\n"
                               "X-Forwarded-For-Y: 189.48.216.61\r\n"
                               "X-Source-Port-Y: 53465\r\n"
                               "X-Start-Time: 1379912431064704\r\n"
                               "X-Req-Id: 1379912431064704-15672417911069840626\r\n"
                               "\r\n";

        THolder<TRequest> req(CreateDummyParsedRequest(REQUEST, GetClassifier()));
        UNIT_ASSERT(!req->IsSearch);
    }
}

Y_UNIT_TEST(TestCaptchaQuery) {
    {
        const TString REQUEST = "GET /search?text=Капчу! HTTP/1.1\r\n"
            "User-Agent: Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; Trident/6.0)\r\n"
            "Accept: text/html, application/xhtml+xml, */*\r\n"
            "Accept-Language: ru-RU\r\n"
            "Host: yandex.ru\r\n"
            "Connection: Keep-Alive\r\n"
            "X-Forwarded-For-Y: 189.48.216.61\r\n"
            "X-Source-Port-Y: 53465\r\n"
            "X-Start-Time: 1379912431064704\r\n"
            "X-Req-Id: 1379912431064704-15672417911069840626\r\n"
            "\r\n";

        THolder<TRequest> req(CreateDummyParsedRequest(REQUEST, GetClassifier()));
        UNIT_ASSERT(req->ForceShowCaptcha);
    }
    {
        const TString REQUEST = "GET /search?query=Капчу! HTTP/1.1\r\n"
            "User-Agent: Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; Trident/6.0)\r\n"
            "Accept: text/html, application/xhtml+xml, */*\r\n"
            "Accept-Language: ru-RU\r\n"
            "Host: yandex.ru\r\n"
            "Connection: Keep-Alive\r\n"
            "X-Forwarded-For-Y: 189.48.216.61\r\n"
            "X-Source-Port-Y: 53465\r\n"
            "X-Start-Time: 1379912431064704\r\n"
            "X-Req-Id: 1379912431064704-15672417911069840626\r\n"
            "\r\n";

        THolder<TRequest> req(CreateDummyParsedRequest(REQUEST, GetClassifier()));
        UNIT_ASSERT(req->ForceShowCaptcha);
    }
}

Y_UNIT_TEST(TestHostTypeForCaptchaRequests) {
    const auto getRequest = [](const TString& host, const TString& url, const TMaybe<TString>& serviceHeader = Nothing()) {
        return "GET " + url + " HTTP/1.1\r\n"
               "User-Agent: Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; Trident/6.0)\r\n"
               "Accept: text/html, application/xhtml+xml, */*\r\n"
               "Accept-Language: ru-RU\r\n"
               "Host: " + host + "\r\n"
               "Connection: Keep-Alive\r\n" +
               (serviceHeader.Defined() ? "X-Antirobot-Service-Y: " + serviceHeader.GetRef() + "\r\n" : "") +
               "X-Forwarded-For-Y: 189.48.216.61\r\n"
               "X-Source-Port-Y: 53465\r\n"
               "X-Start-Time: 1379912431064704\r\n"
               "X-Req-Id: 1379912431064704-15672417911069840626\r\n"
               "\r\n";
    };
    
    {
        auto request = getRequest("yandex.ru", "/showcaptcha?cc=1&retpath=aHR0cDovL3lhbmRleC5ydS9zZWFyY2g/dGV4dD0lRDAlOUElRDAlQjAlRDAlQkYlRDElODclRDElODMlMjE=_29f0a0bfc4d761500760fbe95183fdfd&t=0/1559088303/02bdb39688de61d06717f1911db8ccda&s=ebf1997641c566c5d0fdcfe40050303b");
        THolder<TRequest> req(CreateDummyParsedRequest(request, GetClassifier()));
        UNIT_ASSERT_VALUES_EQUAL(req->HostType, EHostType::HOST_WEB);
    }
    
    {
        auto request = getRequest("auto.ru", "/showcaptcha?cc=1&retpath=aHR0cDovL2F1dG8ucnUvc2VhcmNoP3RleHQ9JUQwJTlBJUQwJUIwJUQwJUJGJUQxJTg3JUQxJTgzJTIx_017fe674e8fce66d3683a623f6a8aba4&t=0/1559088304/df6fa1e25bda500dd7b9f98a22fe621e&s=83446b94fd52c2033fa3714a15fa041c");
        THolder<TRequest> req(CreateDummyParsedRequest(request, GetClassifier()));
        UNIT_ASSERT_VALUES_EQUAL(req->HostType, EHostType::HOST_AUTORU);
    }
    
    {
        auto request = getRequest("kinopoisk.ru", "/showcaptcha?cc=1&retpath=aHR0cDovL2tpbm9wb2lzay5ydS9zZWFyY2g/dGV4dD0lRDAlOUElRDAlQjAlRDAlQkYlRDElODclRDElODMlMjE=_bca4f9f99238e258bfb5063ed07c1369&t=0/1559088304/4604722ea801e92a863a28c90ae13160&s=a65cd5ecda707b03e1071366ff452125");
        THolder<TRequest> req(CreateDummyParsedRequest(request, GetClassifier()));
        UNIT_ASSERT_VALUES_EQUAL(req->HostType, EHostType::HOST_KINOPOISK);
    }
    
    {
        auto request = getRequest("yandex.ru", "/showcaptcha?cc=1&retpath=aHR0cDovL3lhbmRleC5ydS9pbWFnZXMvc2VhcmNoP3RleHQ9JUQwJTlBJUQwJUIwJUQwJUJGJUQxJTg3JUQxJTgzJTIx_f187311eceadc97e3e1741120ddfb9e4&t=0/1559088304/523d79eca9cc2d71734053c88fac32c7&s=e0d1e9fcb8bf279741e4f8d14a0a1375");
        THolder<TRequest> req(CreateDummyParsedRequest(request, GetClassifier()));
        UNIT_ASSERT_VALUES_EQUAL(req->HostType, EHostType::HOST_IMAGES);
    }
    
    {
        auto request = getRequest("beru.ru", "/showcaptcha?cc=1&retpath=aHR0cDovL2JlcnUucnUvc2VhcmNoP3RleHQ9JUQwJTlBJUQwJUIwJUQwJUJGJUQxJTg3JUQxJTgzJTIx_17ff44bd5e4720e13f04e67aba6f356d&t=0/1559088304/5b1bf287a391c740bc0ed21fdba33290&s=408d116dbeda46fd4f0af5176b016b24", "marketblue");
        THolder<TRequest> req(CreateDummyParsedRequest(request, GetClassifier()));
        UNIT_ASSERT_VALUES_EQUAL(req->HostType, EHostType::HOST_MARKETBLUE);
    }
    
    {
        auto request = getRequest("yandex.ru", "/checkcaptcha?key=10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF_0/1559088756/e5f96231457fdc314b8527a81bc5d096_37922968b181ff280b7dd09a8a54d99d&rep=zhest&retpath=aHR0cDovL3lhbmRleC5ydS9zZWFyY2g/dGV4dD0lRDAlOUElRDAlQjAlRDAlQkYlRDElODclRDElODMlMjE=_29f0a0bfc4d761500760fbe95183fdfd");
        THolder<TRequest> req(CreateDummyParsedRequest(request, GetClassifier()));
        UNIT_ASSERT_VALUES_EQUAL(req->HostType, EHostType::HOST_WEB);
    }
    
    {
        auto request = getRequest("auto.ru", "/checkcaptcha?key=10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF_0/1559088757/6e25bc4d1a1e73f661de7c742cdc4cf7_b194dcd631080944ac8e5a814eab87b1&rep=zhest&retpath=aHR0cDovL2F1dG8ucnUvc2VhcmNoP3RleHQ9JUQwJTlBJUQwJUIwJUQwJUJGJUQxJTg3JUQxJTgzJTIx_017fe674e8fce66d3683a623f6a8aba4");
        THolder<TRequest> req(CreateDummyParsedRequest(request, GetClassifier()));
        UNIT_ASSERT_VALUES_EQUAL(req->HostType, EHostType::HOST_AUTORU);
    }
    
    {
        auto request = getRequest("kinopoisk.ru", "/checkcaptcha?key=10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF_0/1559088757/0e3bf3a6175dd7e7551ac93279038ce4_50c14fb20302920507cd4976d20fd8ca&rep=zhest&retpath=aHR0cDovL2tpbm9wb2lzay5ydS9zZWFyY2g/dGV4dD0lRDAlOUElRDAlQjAlRDAlQkYlRDElODclRDElODMlMjE=_bca4f9f99238e258bfb5063ed07c1369");
        THolder<TRequest> req(CreateDummyParsedRequest(request, GetClassifier()));
        UNIT_ASSERT_VALUES_EQUAL(req->HostType, EHostType::HOST_KINOPOISK);
    }
    
    {
        auto request = getRequest("yandex.ru", "/checkcaptcha?key=10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF_0/1559088757/e9847a73cbee39e93b19a797c271bb4f_e338a00be552ee166bab2b9d4b9ac1a1&rep=zhest&retpath=aHR0cDovL3lhbmRleC5ydS9pbWFnZXMvc2VhcmNoP3RleHQ9JUQwJTlBJUQwJUIwJUQwJUJGJUQxJTg3JUQxJTgzJTIx_f187311eceadc97e3e1741120ddfb9e4");
        THolder<TRequest> req(CreateDummyParsedRequest(request, GetClassifier()));
        UNIT_ASSERT_VALUES_EQUAL(req->HostType, EHostType::HOST_IMAGES);
    }
    
    {
        auto request = getRequest("beru.ru", "/checkcaptcha?key=10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF_0/1559088757/b1e772726873751816913d8da691bb70_432144960e4067f88a317f469034a70a&rep=zhest&retpath=aHR0cDovL2JlcnUucnUvc2VhcmNoP3RleHQ9JUQwJTlBJUQwJUIwJUQwJUJGJUQxJTg3JUQxJTgzJTIx_17ff44bd5e4720e13f04e67aba6f356d", "marketblue");
        THolder<TRequest> req(CreateDummyParsedRequest(request, GetClassifier()));
        UNIT_ASSERT_VALUES_EQUAL(req->HostType, EHostType::HOST_MARKETBLUE);
    }
}

Y_UNIT_TEST(TestYandexUidAndSpravkaOnTranslate) {
    const TString REQUEST_WITH_COOKIE = "GET /blablabla HTTP/1.1\r\n"
                            "Host: translate.yandex.ru\r\n"
                            "Cookie: yandexuid=8865387721327286324; spravka=dD0xMzczOTAzODIyO2k9MTI3LjAuMC4xO3U9MTM3MzkwMzgyMjIyNDM0MzA2MTtoPTZjZmYxN2JlM2E3MThiN2UzNDJmOTVlMDI2NzU2MjI0\r\n"
                            "X-Forwarded-For-Y: 85.173.152.104\r\n"
                            "X-Source-Port-Y: 35333\r\n"
                            "X-Start-Time: 1396608694671509\r\n"
                            "X-Req-Id: 1396608694671509-6076052061774650124\r\n"
                            "\r\n";
    const TString REQUEST_WITH_CGI = "GET /blablabla?yu=8865387721327286324&spravka=dD0xMzczOTAzODIyO2k9MTI3LjAuMC4xO3U9MTM3MzkwMzgyMjIyNDM0MzA2MTtoPTZjZmYxN2JlM2E3MThiN2UzNDJmOTVlMDI2NzU2MjI0 HTTP/1.1\r\n"
                            "Host: translate.yandex.ru\r\n"
                            "X-Forwarded-For-Y: 85.173.152.104\r\n"
                            "X-Source-Port-Y: 35333\r\n"
                            "X-Start-Time: 1396608694671509\r\n"
                            "X-Req-Id: 1396608694671509-6076052061774650124\r\n"
                            "\r\n";
    TAutoPtr<TRequest> req_with_cookie = CreateDummyParsedRequest(REQUEST_WITH_COOKIE, GetClassifier());
    UNIT_ASSERT_VALUES_EQUAL(req_with_cookie->HostType, EHostType::HOST_TRANSLATE);
    UNIT_ASSERT_VALUES_EQUAL(req_with_cookie->Uid.Ns, TUid::SPRAVKA);
    UNIT_ASSERT_VALUES_EQUAL(req_with_cookie->YandexUid, "8865387721327286324");
    TAutoPtr<TRequest> req_with_cgi = CreateDummyParsedRequest(REQUEST_WITH_CGI, GetClassifier());
    UNIT_ASSERT_VALUES_EQUAL(req_with_cgi->HostType, EHostType::HOST_TRANSLATE);
    UNIT_ASSERT_VALUES_EQUAL(req_with_cgi->Uid.Ns, TUid::SPRAVKA);
    UNIT_ASSERT_VALUES_EQUAL(req_with_cgi->YandexUid, "8865387721327286324");
}
}

}
