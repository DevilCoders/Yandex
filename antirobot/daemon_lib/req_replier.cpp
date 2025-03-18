#include "req_replier.h"
#include "user_reply.h"

namespace NAntiRobot {

bool TReqReplier::DoReply(const TReplyParams& params) {
    if (!Delayed) {
        WaitTime.Clear();
        auto requesterAddress = GetRequesterAddress(Socket());

        params.Output.EnableKeepAlive(true);

        try {
            auto req = MakeHolder<THttpInfo>(params.Input, requesterAddress,
                                             RequestTimeStats.ReadTimeStats, FailOnReadRequestTimeout, false);

            RequestContext = MakeHolder<TRequestContext>(Env, req.Release());
            Response = ReqHandler(*RequestContext);
        } catch (const THttpInfo::TTimeoutException&) {
            HandleTimeoutException(params);
            Finalize(params);
            return true;
        }
    }

    if (!Response.HasValue() && !Response.HasException()) {
        Delayed = true;
        auto callback = [this](const NThreading::TFuture<TResponse>&) {
            static_cast<IObjectInQueue*>(this)->Process(nullptr);
        };
        Response.Subscribe(callback);
        return false;
    }

    try {
        const auto response = Response.GetValue();

        if (response.HasDisableCompressionHeader()) {
            params.Output.EnableCompressionHeader(false);
        }

        params.Output << response;

        if (response.HadExternalRequests()) {
            Duration->ResetStats(*RequestTimeStats.CaptchaAnswerTimeStats);
        }
    } catch (const THttpInfo::TTimeoutException&) {
        HandleTimeoutException(params);
    } catch (...) {
        HandleGeneralException(params);
    }

    try {
        Finalize(params);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << '\n';
        Env.IncreaseReplyFails();
    }
    return true;
}

void TReqReplier::HandleTimeoutException(const TReplyParams& params) const {
    EVLOG_MSG << EVLOG_ERROR << CurrentExceptionMessage();
    Env.IncreaseTimeouts();
    auto response = NOT_A_ROBOT;
    response.AddHeader(FORWARD_TO_USER_HEADER, false);
    params.Output << response;
}

void TReqReplier::HandleGeneralException(const TReplyParams& params) const {
    EVLOG_MSG << EVLOG_ERROR << CurrentExceptionMessage();
    Env.IncreaseReplyFails();
    auto response = NOT_A_ROBOT;
    response.AddHeader(FORWARD_TO_USER_HEADER, false);
    params.Output << response;
}

} // namespace NAntiRobot
