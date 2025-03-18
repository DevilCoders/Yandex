#include "captcha_page.h"
#include "captcha_page_params.h"
#include "environment.h"
#include "fullreq_info.h"

#include <antirobot/daemon_lib/ut/utils.h>

#include <antirobot/lib/keyring.h>

#include <utility>

using namespace NAntiRobot;

THolder<TRequest> CreateSearchRequestContext(TEnv& env) {
    const TString get = "GET /search?text=cats HTTP/1.1\r\n"
                        "User-Agent: Opera/9.80 (Windows NT 5.1) Presto/2.12.388 Version/12.11\r\n"
                        "Host: yandex.ru\r\n"
                        "Accept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, */*;q=0.1\r\n"
                        "Accept-Language: ru-RU,ru;q=0.9,en;q=0.8\r\n"
                        "Accept-Encoding: gzip, deflate\r\n"
                        "Referer: http://yandex.ru/yandsearch?p=2&text=nude+%22Dany+Carrel%22&clid=47639&lr=48\r\n"
                        "Cookie: yandexuid=345234523456456;\r\n"
                        "Connection: Keep-Alive\r\n"
                        "X-Forwarded-For-Y: 1.1.1.1\r\n"
                        "X-Source-Port-Y: 54542\r\n"
                        "X-Start-Time: 1354193658054839\r\n"
                        "X-Req-Id: 1354193658054839-6563412409256195394\r\n"
                        "\r\n";

    TStringInput stringInput{get};
    THttpInput input{&stringInput};

    return MakeHolder<TFullReqInfo>(input, "", "0.0.0.0", env.ReloadableData, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
}

TCaptchaDescriptor GenCaptcha(TString captchaType = "") {
    TCaptchaToken token(ECaptchaType::SmartAdvanced, "");
    TCaptchaKey key{
        .Token = token,
        .Host = "https://ya.ru",
    };
    return TCaptchaDescriptor{
        .Key = key,
        .CaptchaType = std::move(captchaType)
    };
}

Y_UNIT_TEST_SUITE_IMPL(TTestCaptchaPage, TTestAntirobotMediumBase) {
    Y_UNIT_TEST(TestReqId) {
        TEnv env;
        auto req = CreateSearchRequestContext(env);
        TRequestContext rc{env, req.Release()};
        TCaptchaDescriptor captcha = GenCaptcha();
        TCaptchaPageParams pageParams(rc, captcha, false);
        {
            TCaptchaPage page("%REQ_ID%", "ru");
            UNIT_ASSERT_STRINGS_EQUAL(page.Gen(pageParams), "1354193658054839-6563412409256195394");
        }
    }
}
