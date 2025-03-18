#include <library/cpp/testing/unittest/registar.h>

#include "ar_utils.h"
#include "keyring.h"
#include "spravka.h"
#include "spravka_key.h"

#include <library/cpp/http/cookies/cookies.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/random/random.h>
#include <util/string/hex.h>

namespace NAntiRobot {

class TTestSpravkaBase : public TTestBase {
public:
    void SetUp() override {
        const TString spravkaKey = "4a1faf3281028650e82996f37aada9d97ef7c7c4f3c71914a7cc07a1cbb02d00";
        TStringInput spravkaSI(spravkaKey);
        TSpravkaKey::SetInstance(TSpravkaKey(spravkaSI));

        const TString testKey = "102c46d700bed5c69ed20b7473886468";
        TStringInput keys(testKey);
        TKeyRing::SetInstance(TKeyRing(keys));
    }
};

Y_UNIT_TEST_SUITE_IMPL(TTestSpravka, TTestSpravkaBase) {
    Y_UNIT_TEST(TestSimple) {
        const THttpCookies cookies(TStringBuf("spravka=dD0xMTAwMDAwMDAwO2k9MTIzLjEyLjM0LjQ1O3U9MTIzMTIyNDUzNzg7aD0yMDM1MDI3MDExZWFhNTg4ZTUxYWRiMTk0MzAxNmVmNA=="));

        TSpravka spravka;

        UNIT_ASSERT(spravka.ParseCookies(cookies, "yandex.ru") == TSpravka::ECookieParseResult::Valid);
        UNIT_ASSERT_EQUAL(spravka.Addr, TAddr("123.12.34.45"));
        UNIT_ASSERT_EQUAL(spravka.Uid, ULL(12312245378));

        UNIT_ASSERT(!spravka.Degradation.Web);
        UNIT_ASSERT(!spravka.Degradation.Market);
    }

    Y_UNIT_TEST(TestDomained) {
        // all ip - 127.0.0.1, uid - auto
        const THttpCookies cookiesNotDomained(TStringBuf("spravka=dD0xMzczOTAzODIyO2k9MTI3LjAuMC4xO3U9MTM3MzkwMzgyMjIyMzU3NzE5MDtoPWEwZjcyMTcwMGEyMGRhMzFkZmI0MzI4YTgzN2E4NTQ4"));
        const THttpCookies cookiesYandexRu(TStringBuf("spravka=dD0xMzczOTAzODIyO2k9MTI3LjAuMC4xO3U9MTM3MzkwMzgyMjIyNDM0MzA2MTtoPTZjZmYxN2JlM2E3MThiN2UzNDJmOTVlMDI2NzU2MjI0"));
        const THttpCookies cookiesYandexComTr(TStringBuf("spravka=dD0xMzczOTAzODIyO2k9MTI3LjAuMC4xO3U9MTM3MzkwMzgyMjIyNDI0OTQ1NjtoPTg5YjlmMjMxYjNiYmMzYzI0OGJjMDQxYTg4ZTViY2Yx"));
        TSpravka spravkaNotDomained;
        TSpravka spravkaYandexRu;
        TSpravka spravkaYandexComTr;

        UNIT_ASSERT(spravkaYandexRu.ParseCookies(cookiesYandexRu, TStringBuf()) == TSpravka::ECookieParseResult::Invalid);
        UNIT_ASSERT(spravkaYandexRu.ParseCookies(cookiesYandexRu, TStringBuf("yandex.ru")) == TSpravka::ECookieParseResult::Valid);
        UNIT_ASSERT(spravkaYandexRu.ParseCookies(cookiesYandexRu, TStringBuf("yandex.com.tr")) == TSpravka::ECookieParseResult::Invalid);
        UNIT_ASSERT(spravkaYandexRu.ParseCookies(cookiesYandexRu, TStringBuf("yandex.com")) == TSpravka::ECookieParseResult::Invalid);

        UNIT_ASSERT(spravkaYandexComTr.ParseCookies(cookiesYandexComTr, TStringBuf()) == TSpravka::ECookieParseResult::Invalid);
        UNIT_ASSERT(spravkaYandexComTr.ParseCookies(cookiesYandexComTr, TStringBuf("yandex.ru")) == TSpravka::ECookieParseResult::Invalid);
        UNIT_ASSERT(spravkaYandexComTr.ParseCookies(cookiesYandexComTr, TStringBuf("yandex.com.tr")) == TSpravka::ECookieParseResult::Valid);
        UNIT_ASSERT(spravkaYandexComTr.ParseCookies(cookiesYandexComTr, TStringBuf("yandex.com")) == TSpravka::ECookieParseResult::Invalid);
    }

    Y_UNIT_TEST(TestEmptyDomain) {
        // Справка, выданная с пустым доменом, не валидна
        const THttpCookies cookies(TStringBuf("spravka=dD0xMTAwMDAwMDAwO2k9MTIzLjEyLjM0LjQ1O3U9MTIzMTIyNDUzNzg7aD0wMTU5ZmRkZDMwZDFhYzI1OTQ0ODJjNWI1ZDViZjE2Nw=="));

        TSpravka spravka;

        UNIT_ASSERT(spravka.ParseCookies(cookies, TStringBuf()) == TSpravka::ECookieParseResult::Invalid);
        UNIT_ASSERT(spravka.ParseCookies(cookies, TStringBuf("yandex.ru")) == TSpravka::ECookieParseResult::Invalid);
    }

    Y_UNIT_TEST(TestEscaped) {
        const THttpCookies cookies(TStringBuf("spravka=dD0xMjc4NTAyMDM0O2k9ODcuMjUwLjI0OS4yMzc7dT0xMjc4NTAyMDM0MDQ1NTY0NDQwO2g9NDIwYjA1ODViNzMzZTA3MWRjNGFhNDZhMTYxOGI2ZGI%3D"));

        TSpravka spravka;

        UNIT_ASSERT(spravka.ParseCookies(cookies, "yandex.ru") == TSpravka::ECookieParseResult::Valid);
        UNIT_ASSERT_EQUAL(spravka.Addr, TAddr("87.250.249.237"));
        UNIT_ASSERT_EQUAL(spravka.Uid, ULL(1278502034045564440));
        UNIT_ASSERT_EQUAL(spravka.Time, TInstant::Seconds(1278502034));

        UNIT_ASSERT_EQUAL(TSpravka::GetTime(spravka.Uid).GetValue(), ULL(1278502034045000));

        UNIT_ASSERT(!spravka.Degradation.Web);
        UNIT_ASSERT(!spravka.Degradation.Market);
    }

    Y_UNIT_TEST(TestEmptySpravka) {
        const THttpCookies cookies(TStringBuf("aaa=bbb"));

        TSpravka spravka;

        UNIT_ASSERT(spravka.ParseCookies(cookies, "yandex.ru") == TSpravka::ECookieParseResult::NotFound);
    }

    Y_UNIT_TEST(TestInvalid) {
        TSpravka spravka;

        const THttpCookies cookies1(TStringBuf("spravka=-2383249832498237498234982347983274982349823"));
        UNIT_ASSERT(spravka.ParseCookies(cookies1, "yandex.ru") == TSpravka::ECookieParseResult::Invalid);

        TString spravkaStr2 = TString(TSpravka::NAMES[0]) + "=" + Base64Encode(TStringBuf("h=123"));
        const THttpCookies cookies2(TStringBuf(spravkaStr2.data(), spravkaStr2.size()));
        UNIT_ASSERT(spravka.ParseCookies(cookies2, "yandex.ru") == TSpravka::ECookieParseResult::Invalid);
    }

    Y_UNIT_TEST(TestInvalidBase64Sign) {
        TSpravka spravka;

        const THttpCookies cookies1(TStringBuf("spravka=dD0xNjM0NTQzOTM3O2k9MmEwMjo2Yjg6YjA4MTo4OTIxOjoxOjE7RD00QjlDRkRBNUIxNzAzMTcyOEYyMUQ5QkYxM0RGOEZCNjcyQzczMDc0RTcwRTUxNjBBRTRCNDU3MzY2N0Q0MDQ1QTFFNzt1PTE2MzQ1NDM5Mzc3MDIzMTA3MzM7aD16Wnpaelp6Wnpaelp6Wnpaelp6Wnpaelp6Wnpaelp6Wg=="));
        UNIT_ASSERT(spravka.ParseCookies(cookies1, "yandex.ru") == TSpravka::ECookieParseResult::Invalid);
    }

    Y_UNIT_TEST(TestGenerateSpravka) {
        constexpr TStringBuf domain = "yandex.ru";

        for (size_t i = 0; i < 10; ++i) {
            ui32 ip = RandomNumber<ui32>();
            TSpravka::TDegradation degradation;
            TString cookiesStr = TString(TSpravka::NAMES[0]) + "=" + TSpravka::Generate(TAddr(IpToStr(ip)), domain, degradation).ToString();
            const THttpCookies cookies(cookiesStr);

            TSpravka spravka;

            UNIT_ASSERT(spravka.ParseCookies(cookies, domain) == TSpravka::ECookieParseResult::Valid);
            UNIT_ASSERT_EQUAL(spravka.Addr, TAddr(IpToStr(ip)));
        }

        TSpravka::TDegradation degradation;
        TSpravka spr = TSpravka::Generate(TAddr(IpToStr(123)), TInstant::MicroSeconds(123321), domain, degradation);
        UNIT_ASSERT_EQUAL(spr.Addr, TAddr(IpToStr(123)));
        UNIT_ASSERT_EQUAL(spr.Time.GetValue(), 123321);
        UNIT_ASSERT_EQUAL(TSpravka::GetTime(spr.Uid).GetValue(), 123000);
        UNIT_ASSERT(!spr.Degradation.Web);
        UNIT_ASSERT(!spr.Degradation.Market);
        UNIT_ASSERT(!spr.Degradation.Uslugi);
        UNIT_ASSERT(!spr.Degradation.Autoru);
    }

    Y_UNIT_TEST(TestIPv6) {
        TStringBuf addr = "102:304:506:1708:90a:2b0c:d0e:f10";
        TStringBuf domain = "yandex.ru";

        TSpravka::TDegradation degradation;
        degradation.Market = true;
        TSpravka spr = TSpravka::Generate(TAddr(addr), domain, degradation);

        UNIT_ASSERT(spr.Addr.Valid());
        UNIT_ASSERT_EQUAL(spr.Addr.GetFamily(), AF_INET6);
        UNIT_ASSERT_EQUAL(spr.Addr.ToString(), addr);

        TString cookSpravka = spr.AsCookie();

        const  THttpCookies cookies(cookSpravka);

        TSpravka spr2;
        UNIT_ASSERT(spr2.ParseCookies(cookies, domain) == TSpravka::ECookieParseResult::Valid);
        UNIT_ASSERT_EQUAL(spr2.Addr.GetFamily(), AF_INET6);
        UNIT_ASSERT_EQUAL(spr2.Addr.ToString(), addr);
        UNIT_ASSERT(!spr.Degradation.Web);
        UNIT_ASSERT(spr.Degradation.Market);
        UNIT_ASSERT(!spr.Degradation.Uslugi);
        UNIT_ASSERT(!spr.Degradation.Autoru);
    }

    Y_UNIT_TEST(TestGetLatestSpravka) {
        const THttpCookies cookies1("spravka=dD0xMjk0NjI1NDE3O2k9OTIuMzkuNTMuMTQwO3U9MTI5NDYyNTQxNzg4MDI5MzEzMjtoPTU2NDlkMjdhMjc3NmE0YmI3NTYzYzc2MjRiNDgyMDgz; spravka=dD0xMjk0NjI1NTQxO2k9OTIuMzkuNTMuMTQwO3U9MTI5NDYyNTU0MTkyOTMxNTkzOTtoPTc4NTZiNjhhZDA3MGIyZjg2ZmMyMDI0YzEyZDA4NjRi;;Session_id=noauth:9993284776;");
        TSpravka spravka1;
        constexpr TStringBuf domain = "yandex.ru";

        UNIT_ASSERT(spravka1.ParseCookies(cookies1, domain) == TSpravka::ECookieParseResult::Valid);
        UNIT_ASSERT_EQUAL(spravka1.Addr, TAddr("92.39.53.140"));
        UNIT_ASSERT_EQUAL(spravka1.Uid, ULL(1294625541929315939));
        UNIT_ASSERT_EQUAL(spravka1.Time, TInstant::Seconds(1294625541));

        UNIT_ASSERT_EQUAL(TSpravka::GetTime(spravka1.Uid).GetValue(), ULL(1294625541929000));

        UNIT_ASSERT(!spravka1.Degradation.Web);
        UNIT_ASSERT(!spravka1.Degradation.Market);

        // spravkas in different order
        const THttpCookies cookies2("spravka=dD0xMjk0NjI1NTQxO2k9OTIuMzkuNTMuMTQwO3U9MTI5NDYyNTU0MTkyOTMxNTkzOTtoPTc4NTZiNjhhZDA3MGIyZjg2ZmMyMDI0YzEyZDA4NjRi; spravka=dD0xMjk0NjI1NDE3O2k9OTIuMzkuNTMuMTQwO3U9MTI5NDYyNTQxNzg4MDI5MzEzMjtoPTU2NDlkMjdhMjc3NmE0YmI3NTYzYzc2MjRiNDgyMDgz;;Session_id=noauth:9993284776;");
        TSpravka spravka2;

        UNIT_ASSERT(spravka2.ParseCookies(cookies2, domain) == TSpravka::ECookieParseResult::Valid);
        UNIT_ASSERT_EQUAL(spravka2.Addr, TAddr("92.39.53.140"));
        UNIT_ASSERT_EQUAL(spravka2.Uid, ULL(1294625541929315939));
        UNIT_ASSERT_EQUAL(spravka2.Time, TInstant::Seconds(1294625541));

        UNIT_ASSERT_EQUAL(TSpravka::GetTime(spravka2.Uid).GetValue(), ULL(1294625541929000));

        UNIT_ASSERT(!spravka2.Degradation.Web);
        UNIT_ASSERT(!spravka2.Degradation.Market);
    }

    Y_UNIT_TEST(TestDegradationWeb) {
        const TString cookiesStr = "spravka=dD0xMTAwMDAwMDAwO2k9MS4xLjEuMTtEPUYxN0MwQkU0MDAzQzMxRUNBQzhCNjgyRERGRjc1ODVFRDlBNkQ3MTEwRTQ4MTQ3NDlFM0UzNTc3RUJBMjNBMEQ7dT0xMjMxMjI0NTM3ODtoPThlNGQxYjQxMmE4ZDUyMGQxZGM2Mjc1ZTVhMjc4YjE0";
        const THttpCookies cookies(cookiesStr);
        TSpravka spravka;
        constexpr TStringBuf domain = "yandex.ru";

        UNIT_ASSERT(spravka.ParseCookies(cookies, domain) == TSpravka::ECookieParseResult::Valid);
        UNIT_ASSERT_EQUAL(spravka.Addr, TAddr("1.1.1.1"));
        UNIT_ASSERT_EQUAL(spravka.Uid, ULL(12312245378));
        UNIT_ASSERT_EQUAL(spravka.Time, TInstant::Seconds(1100000000));
        UNIT_ASSERT(spravka.Degradation.Web);
        UNIT_ASSERT(!spravka.Degradation.Market);
    }
};

} // namespace NAntiRobot
