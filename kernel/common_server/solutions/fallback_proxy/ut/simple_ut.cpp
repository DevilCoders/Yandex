#include "common.h"
#include "commands/check_queue.h"

using namespace NCS::NFallbackProxy;
using namespace NServerTest;
Y_UNIT_TEST_SUITE(Simple) {
    Y_UNIT_TEST_F(Simple, TFallbackTestBase) {
        OnTestStart();
        NServerTest::TScript script;
        NNeh::THttpRequest request;
        request.SetUri("proxy/test/handler/xxx");
        script.Add<TSendRequest>().SetRequest(request).ExpectCode(200).ExpectJsonPath("income_request.uri", "test/handler/xxx");
        script.Add<TSetSetting>().SetWaiting(true).SetKey("handlers.test/handler/xxx.base_enabled").SetValue(false);
        script.Add<TSendRequest>().SetRequest(request).ExpectCode(503);
        request.SetPostData(TString(R"({"external_content_id" : {"value" : "abcde"}})"));
        script.Add<TSendRequest>().SetRequest(request).ExpectCode(202).ExpectJsonPath("expected_content_id.value", "abcde");
        script.Add<TCheckFallbackQueue>().ExpectSize(1);
        script.Add<TSetSetting>().SetWaiting(true).SetKey("handlers.test/handler/xxx.base_enabled").SetValue(true);
        script.Add<TCheckFallbackQueue>().ExpectSize(0);

        TTestContext context(GetServerGuard(), GetServerClient());
        UNIT_ASSERT(script.Execute(context));
    }

    Y_UNIT_TEST_F(UnknownMessage, TFallbackTestBase) {
        OnTestStart();
        NServerTest::TScript script;

        script.Add<TSetSetting>().SetWaiting(true).SetKey("handlers.test/handler/xxx.base_enabled").SetValue(false);

        NNeh::THttpRequest request;
        request.SetUri("proxy/status/by_message_id");
        request.SetPostData(TString(R"({"status" : {"expected_content_id" : "abcde"}})"));
        script.Add<TSendRequest>().SetRequest(request).ExpectCode(HTTP_SERVICE_UNAVAILABLE);

        TTestContext context(GetServerGuard(), GetServerClient());
        UNIT_ASSERT(script.Execute(context));
    }

    Y_UNIT_TEST_F(SimpleStatus, TFallbackTestBase) {
        OnTestStart();
        NServerTest::TScript script;

        NNeh::THttpRequest requestStatus;
        requestStatus.SetUri("proxy/status/by_message_id");
        script.Add<TSendRequest>().SetRequest(requestStatus).ExpectCode(200);
        script.Add<TCheckFallbackQueue>().ExpectSize(0);

        NNeh::THttpRequest request;
        request.SetUri("proxy/test/handler/xxx");

        script.Add<TSetSetting>().SetWaiting(true).SetKey("handlers.test/handler/xxx.base_enabled").SetValue(false);
        script.Add<TSendRequest>().SetRequest(request).ExpectCode(503);
        request.SetPostData(TString(R"({"external_content_id" : {"value" : "abcde"}})"));
        script.Add<TSendRequest>().SetRequest(request).ExpectCode(202).ExpectJsonPath("expected_content_id.value", "abcde");
        script.Add<TCheckFallbackQueue>().ExpectSize(1);
        requestStatus.SetPostData(TString(R"({"status" : {"expected_content_id" : "abcde"}})"));
        script.Add<TSendRequest>().SetRequest(requestStatus).ExpectCode(203).ExpectJsonPath("status.bbb", "abcde");
        script.Add<TSetSetting>().SetWaiting(true).SetKey("handlers.test/handler/xxx.base_enabled").SetValue(true);
        script.Add<TCheckFallbackQueue>().ExpectSize(0);
        script.Add<TSendRequest>().SetRequest(requestStatus).ExpectCode(200).ExpectJsonPath("income_request.uri", "test/handler/xxx");

        TTestContext context(GetServerGuard(), GetServerClient());
        UNIT_ASSERT(script.Execute(context));
    }
}

Y_UNIT_TEST_SUITE(Heavy) {

    void SendRequestsPack(NServerTest::TScript& script, int requestsCnt, const TString& handler,
                          int expectedCode, std::function<TString(int id)> printPostData) {
        NNeh::THttpRequest request;
        request.SetUri(handler);
        for (int i = 0; i < requestsCnt; ++i) {
            request.SetPostData(printPostData(i));
            script.Add<TSendRequest>().SetRequest(request).ExpectCode(expectedCode);
        }
    }

    Y_UNIT_TEST_F(SimpleFallback, TFallbackTestBase) {
        OnTestStart();
        NServerTest::TScript script;

        script.Add<TSetSetting>().SetWaiting(true).SetKey("handlers.test/handler/xxx.base_enabled").SetValue(false);

        const int kRequests = 2048;
        SendRequestsPack(script, kRequests, "proxy/test/handler/xxx", 202, [](int id) {
            return Sprintf(R"({"external_content_id" : {"value" : "abcde%d"}})", id);
        });
        script.Add<TCheckFallbackQueue>().ExpectSize(kRequests);
        SendRequestsPack(script, kRequests, "proxy/status/by_message_id", 203, [](int id) {
            return Sprintf(R"({"status" : {"expected_content_id" : "abcde%d"}})", id);
        });

        const int kIterations = 16;
        const int kAdditionalRequests = 32;
        for (int i = 0; i < kIterations; ++i) {
            SendRequestsPack(script, kAdditionalRequests + (i % 4) * 8, "proxy/test/handler/xxx", 202, [&i](int id) {
                return Sprintf(R"({"external_content_id" : {"value" : "::%02d::%d"}})", 2 * i, id);
            });

            script.Add<TSetSetting>().SetWaiting(true).SetKey("handlers.test/handler/xxx.base_enabled").SetValue(true);

            SendRequestsPack(script, kAdditionalRequests + (i % 5) * 7, "proxy/test/handler/xxx", 200, [&i](int id) {
                return Sprintf(R"({"external_content_id" : {"value" : "::%02d::%d"}})", 2 * i + 1, id);
            });

            script.Add<TSleepAction>().SetTimeout(TDuration::Seconds(1 + i % 3));
            script.Add<TSetSetting>().SetWaiting(true).SetKey("handlers.test/handler/xxx.base_enabled").SetValue(false);
        }

        script.Add<TCheckFallbackQueue>().ExpectSize(0);

        TTestContext context(GetServerGuard(), GetServerClient());
        UNIT_ASSERT(script.Execute(context));
    }
}
