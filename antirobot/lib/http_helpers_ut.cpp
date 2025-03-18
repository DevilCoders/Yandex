#include <library/cpp/testing/unittest/registar.h>

#include "http_helpers.h"
#include "neh_requester.h"
#include "test_server.h"

#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/http/server/response.h>

#include <util/datetime/base.h>

namespace NAntiRobot {

Y_UNIT_TEST_SUITE(TestFetchHttpDataAsync) {

const TString THE_DATA = "poskj129he3;incqdfcqwn'qw8i0h 820h falknfas;'fih";
const TDuration THE_TIMEOUT = TDuration::MilliSeconds(100);
const size_t MaxNehQueueSize = 10000;

const std::pair<TString, THttpResponse> RESP_DATA[] = {
    std::make_pair("/200", THttpResponse(HTTP_OK)),
    std::make_pair("/302", THttpResponse(HTTP_FOUND)),
    std::make_pair("/data", THttpResponse(HTTP_OK).SetContent(THE_DATA)),
};

const THashMap<TString, THttpResponse> RESPONSES(RESP_DATA, RESP_DATA + Y_ARRAY_SIZE(RESP_DATA));

class TReplier : public TRequestReplier {
public:
    bool DoReply(const TReplyParams& params) override {
        TParsedHttpFull request(params.Input.FirstLine());

        const THttpResponse* resp = RESPONSES.FindPtr(request.Path);
        if (resp) {
            params.Output << *resp;
        } else if (request.Path == "/timeout"sv) {
            Sleep(THE_TIMEOUT * 2);
            params.Output << THttpResponse(HTTP_OK);
        } else {
            params.Output << THttpResponse(HTTP_NOT_FOUND);
        }

        return true;
    }
};

struct TCallback : public THttpServer::ICallBack {
    TClientRequest* CreateClient() override {
        return new TReplier;
    }
};

struct TScopedServer : public TCallback, public TTestServer {
    TScopedServer()
        : TCallback()
        , TTestServer(*static_cast<TCallback*>(this))
    {
    }
};

Y_UNIT_TEST(TestMultipleScopedServers) {
    // we must be able to run multiple servers simultaneously on different ports
    TScopedServer servers[50];
    Y_UNUSED(servers);
}

Y_UNIT_TEST(TestFetchHttpDataSuccess) {
    TScopedServer srv;

    TNehRequester requester(MaxNehQueueSize);
    auto future = FetchHttpDataAsync(&requester, srv.Host, HttpGet(srv.Host, "/data"), THE_TIMEOUT, "http");
    auto answerOrError = future.ExtractValueSync();
    NNeh::TResponseRef answer;
    auto error = answerOrError.PutValueTo(answer);

    UNIT_ASSERT(!error.Defined());
    UNIT_ASSERT_STRINGS_EQUAL(answer->Data, THE_DATA);
}

Y_UNIT_TEST(TestFetchHttpDataFail) {
    TScopedServer srv;

    TNehRequester requester(MaxNehQueueSize);
    auto future = FetchHttpDataAsync(&requester, srv.Host, HttpGet(srv.Host, "/302"), THE_TIMEOUT, "http");
    auto answerOrError = future.ExtractValueSync();
    NNeh::TResponseRef answer;
    auto error = answerOrError.PutValueTo(answer);

    UNIT_ASSERT(!error.Defined());
    UNIT_ASSERT(answer->IsError());
    UNIT_ASSERT_VALUES_EQUAL(answer->GetErrorCode(), 302);
}

Y_UNIT_TEST(TestFetchHttpDataTimeout) {
    TScopedServer srv;

    TNehRequester requester(MaxNehQueueSize);
    auto future = FetchHttpDataAsync(&requester, srv.Host, HttpGet(srv.Host, "/timeout"), THE_TIMEOUT, "http");
    auto answerOrError = future.ExtractValueSync();
    NNeh::TResponseRef answer;
    auto error = answerOrError.PutValueTo(answer);

    UNIT_ASSERT(error.Defined());
    UNIT_ASSERT_EXCEPTION(error.Throw(), TTimeoutException);
}

}

}
