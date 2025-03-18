#include "fullreq_handler.h"

#include <antirobot/daemon_lib/ut/utils.h>

using namespace NAntiRobot;

static THolder<TRequest> CreateSearchRequestContext(TEnv& env) {
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

    auto res = MakeHolder<TFullReqInfo>(input, "", "0.0.0.0", env.ReloadableData, TPanicFlags::CreateFake(), GetEmptySpravkaIgnorePredicate());
    res->ContentData = get;
    return res;
}

Y_UNIT_TEST_SUITE_IMPL(TTestFullreqHandler, TTestAntirobotMediumBase) {
    Y_UNIT_TEST(Test) {
        TEnv env;
        auto req = CreateSearchRequestContext(env);
        TRequestContext rc{env, req.Release()};

        {
            TFullreqHandler handler([](TRequestContext&) {
                return NThreading::MakeFuture(TResponse::ToBalancer(HTTP_OK));
            });
            TResponse resp = handler(rc).GetValueSync();
            const auto& headers = resp.GetHeaders();
            {
                const THttpInputHeader* header = headers.FindHeader("X-Yandex-EU-Request");
                UNIT_ASSERT_UNEQUAL(header, nullptr);
                UNIT_ASSERT_EQUAL_C(header->Value(), "0", header->Value());
            }
            {
                const THttpInputHeader* header = headers.FindHeader("X-Antirobot-Region-Id");
                UNIT_ASSERT_UNEQUAL(header, nullptr);
                UNIT_ASSERT_EQUAL_C(header->Value(), "211", header->Value());
            }
        }
        {
            TFullreqHandler handler([](TRequestContext&) {
                return NThreading::MakeFuture(TResponse::ToUser(HTTP_OK));
            });
            TResponse resp = handler(rc).GetValueSync();
            const auto& headers = resp.GetHeaders();
            {
                const THttpInputHeader* header = headers.FindHeader("X-Yandex-EU-Request");
                UNIT_ASSERT_UNEQUAL(header, nullptr);
                UNIT_ASSERT_EQUAL_C(header->Value(), "0", header->Value());
            }
            for (auto name : {"X-Yandex-Internal-Request", "X-Yandex-Suspected-Robot", "X-Antirobot-Is-Crawler",
                              "X-Antirobot-Suspiciousness-Y", "X-Antirobot-Region-Id", "X-Yandex-Antirobot-Degradation"}) {
                const THttpInputHeader* header = headers.FindHeader(name);
                UNIT_ASSERT_EQUAL_C(header, nullptr, name);
            }
        }
        {
            auto prevCnt = static_cast<size_t>(rc.Env.ServerExceptionsStats.Get(EServerExceptionCounter::BadExpects));
            TFullreqHandler handler([](TRequestContext&) -> NThreading::TFuture<TResponse> {
                throw TFullReqInfo::TBadExpect();
            });
            TResponse resp = handler(rc).GetValueSync();
            const auto& headers = resp.GetHeaders();
            const THttpInputHeader* header = headers.FindHeader("X-Yandex-EU-Request");
            UNIT_ASSERT_UNEQUAL(header, nullptr);
            UNIT_ASSERT_EQUAL_C(header->Value(), "0", header->Value());
            UNIT_ASSERT_EQUAL(prevCnt + 1, static_cast<size_t>(rc.Env.ServerExceptionsStats.Get(EServerExceptionCounter::BadExpects)));
        }
        {
            auto prevCnt = static_cast<size_t>(rc.Env.ServerExceptionsStats.Get(EServerExceptionCounter::UidCreationFailures));
            TFullreqHandler handler([](TRequestContext&) -> NThreading::TFuture<TResponse> {
                throw TFullReqInfo::TUidCreationFailure();
            });
            TResponse resp = handler(rc).GetValueSync();
            const auto& headers = resp.GetHeaders();
            const THttpInputHeader* header = headers.FindHeader("X-Yandex-EU-Request");
            UNIT_ASSERT_UNEQUAL(header, nullptr);
            UNIT_ASSERT_EQUAL_C(header->Value(), "0", header->Value());
            UNIT_ASSERT_EQUAL(prevCnt + 1, static_cast<size_t>(rc.Env.ServerExceptionsStats.Get(EServerExceptionCounter::UidCreationFailures)));
        }
    }
}
