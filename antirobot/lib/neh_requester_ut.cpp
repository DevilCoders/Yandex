#include <library/cpp/testing/unittest/registar.h>

#include "neh_requester.h"
#include "test_server.h"

#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/http/server/response.h>

namespace NAntiRobot {
    namespace {
        const TDuration DEFAULT_TIMEOUT = TDuration::MilliSeconds(500);
        const TString DEFAULT_ANSWER = "ANSWER";
        const size_t MaxNehQueueSize = 10000;

        struct TLocationChecker: public TRequestReplier {
            TString Location;

            TLocationChecker(TString location)
                : Location(location)
            {
            }

            bool DoReply(const TReplyParams& params) override {
                TParsedHttpFull request(params.Input.FirstLine());
                if (request.Path == Location) {
                    params.Output << THttpResponse(HTTP_OK).SetContent(DEFAULT_ANSWER);
                } else {
                    params.Output << THttpResponse(HTTP_NOT_FOUND);
                }
                return true;
            }

            TLocationChecker* Clone() const {
                return new TLocationChecker(Location);
            }
        };

        template <typename ClientRequest>
        struct TCloneCallback: public THttpServer::ICallBack {
            THolder<ClientRequest> Replier;

            TCloneCallback(THolder<ClientRequest> replier)
                : Replier(std::move(replier))
            {
            }

            TClientRequest* CreateClient() override {
                return Replier->Clone();
            }
        };

        struct TTimeouter: public TRequestReplier {
            TDuration SleepBeforeReply;

            TTimeouter(TDuration sleepBeforeReply)
                : SleepBeforeReply(sleepBeforeReply)
            {
            }

            bool DoReply(const TReplyParams& params) override {
                Sleep(SleepBeforeReply);
                params.Output << THttpResponse(HTTP_OK).SetContent(DEFAULT_ANSWER);
                return true;
            }

            TRequestReplier* Clone() const {
                return new TTimeouter(SleepBeforeReply);
            }
        };

        struct TAlwaysBadRequest: public TRequestReplier {
            bool DoReply(const TReplyParams& params) override {
                params.Output << THttpResponse(HTTP_BAD_REQUEST);
                return true;
            }

            TRequestReplier* Clone() const {
                return new TAlwaysBadRequest;
            }
        };
    }

    Y_UNIT_TEST_SUITE(NehRequester) {
        Y_UNIT_TEST(HTTP_200_OK) {
            const TString location = "/test";
            TCloneCallback<TLocationChecker> callback(MakeHolder<TLocationChecker>(location));
            TTestServer server(callback);
            TNehRequester requester(MaxNehQueueSize);
            auto message = NNeh::TMessage::FromString("http://localhost:" + ToString(server.Host.Port) + location);

            auto future = requester.RequestAsync(message, TDuration::Max());
            auto answerOrError = future.ExtractValueSync();
            NNeh::TResponseRef answer;
            auto error = answerOrError.PutValueTo(answer);

            UNIT_ASSERT(!error.Defined());
            UNIT_ASSERT(!answer->IsError());
            UNIT_ASSERT_STRINGS_EQUAL(answer->Data, DEFAULT_ANSWER);
        }

        Y_UNIT_TEST(HTTP_404_NOT_FOUND) {
            const TString location = "/test";
            TCloneCallback<TLocationChecker> callback(MakeHolder<TLocationChecker>(location));
            TTestServer server(callback);
            TNehRequester requester(MaxNehQueueSize);
            auto message = NNeh::TMessage::FromString("http://localhost:" + ToString(server.Host.Port) + "/not_found");

            auto future = requester.RequestAsync(message, DEFAULT_TIMEOUT);
            auto answerOrError = future.ExtractValueSync();
            NNeh::TResponseRef answer;
            auto error = answerOrError.PutValueTo(answer);

            UNIT_ASSERT(!error.Defined());
            UNIT_ASSERT(answer->IsError());
            UNIT_ASSERT_VALUES_EQUAL(answer->GetErrorCode(), HTTP_NOT_FOUND);
        }

        Y_UNIT_TEST(HTTP_400_BAD_REQUEST) {
            TCloneCallback<TAlwaysBadRequest> callback(THolder(new TAlwaysBadRequest));
            TTestServer server(callback);

            TNehRequester requester(MaxNehQueueSize);
            auto message = NNeh::TMessage::FromString("http://localhost:" + ToString(server.Host.Port) + "/test");

            auto future = requester.RequestAsync(message, DEFAULT_TIMEOUT);
            auto answerOrError = future.ExtractValueSync();
            NNeh::TResponseRef answer;
            auto error = answerOrError.PutValueTo(answer);

            UNIT_ASSERT(!error.Defined());
            UNIT_ASSERT(answer->IsError());
            UNIT_ASSERT_VALUES_EQUAL(answer->GetErrorCode(), HTTP_BAD_REQUEST);
        }

        Y_UNIT_TEST(Timeout) {
            TCloneCallback<TTimeouter> callback(MakeHolder<TTimeouter>(DEFAULT_TIMEOUT * 2));
            TTestServer server(callback);
            TNehRequester requester(MaxNehQueueSize);
            auto message = NNeh::TMessage::FromString("http://localhost:" + ToString(server.Host.Port) + "/test");

            auto future = requester.RequestAsync(message, DEFAULT_TIMEOUT);
            auto answerOrError = future.ExtractValueSync();
            NNeh::TResponseRef answer;
            auto error = answerOrError.PutValueTo(answer);

            UNIT_ASSERT(error.Defined());
            UNIT_ASSERT_EXCEPTION(error.Throw(), TTimeoutException);
        }

        Y_UNIT_TEST(BadProtocol) {
            TNehRequester requester(MaxNehQueueSize);
            auto message = NNeh::TMessage::FromString("bad_proto://localhost:1234");

            auto future = requester.RequestAsync(message, DEFAULT_TIMEOUT);
            auto answerOrError = future.ExtractValueSync();

            UNIT_ASSERT(answerOrError.HasError());
            UNIT_ASSERT_EXCEPTION_CONTAINS(answerOrError.ReleaseError().Throw(), yexception, "unsupported scheme");
        }

        Y_UNIT_TEST(QueueOverflow) {
            const TString location = "/test";
            TCloneCallback<TLocationChecker> callback(MakeHolder<TLocationChecker>(location));
            TTestServer server(callback);
            TNehRequester requester(/* maxQueueSize := */ 0);
            auto message = NNeh::TMessage::FromString("http://localhost:" + ToString(server.Host.Port) + location);

            auto future = requester.RequestAsync(message, TDuration::Max());
            auto answerOrError = future.ExtractValueSync();

            UNIT_ASSERT(answerOrError.HasError());
            UNIT_ASSERT_EXCEPTION(answerOrError.ReleaseError().Throw(), TNehQueueOverflowException);
        }
    }

}
