#include "captcha_req_replier.h"

#include "eventlog_err.h"
#include "fullreq_info.h"
#include "time_stats.h"
#include "unified_agent_log.h"

#include <util/string/printf.h>

namespace NAntiRobot {

bool TCaptchaReqReplier::DoReply(const TReplyParams& params) {
    if (!Delayed) {
        WaitTime.Clear();
        auto requesterAddress = GetRequesterAddress(Socket());

        params.Output.EnableKeepAlive(true);

        try {
            auto req = MakeHolder<TFullReqInfo>(
                params.Input,
                "",
                requesterAddress,
                Env.ReloadableData,
                Env.PanicFlags,
                GetEmptySpravkaIgnorePredicate(),
                &Env.ReqGroupClassifier,
                &Env
            );
            Y_UNUSED(FailOnReadRequestTimeout); // TODO: pass  RequestTimeStats.ReadTimeStats, FailOnReadRequestTimeout;

            RequestContext = MakeHolder<TRequestContext>(Env, req.Release());
            Response = ReqHandler(*RequestContext);
        } catch (const THttpInfo::TTimeoutException&) {
            HandleTimeoutException(params);
            Finalize(params);
            return true;
        } catch (...) {
            HandleGeneralException(params);
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

void TCaptchaReqReplier::HandleTimeoutException(const TReplyParams& params) const {
    EVLOG_MSG << EVLOG_ERROR << CurrentExceptionMessage();
    if (ANTIROBOT_DAEMON_CONFIG.DebugOutput) {
        Cerr << CurrentExceptionMessage() << Endl;
    }
    Env.IncreaseTimeouts();
    params.Output << TResponse::ToUser(HTTP_INTERNAL_SERVER_ERROR);
}

void TCaptchaReqReplier::HandleGeneralException(const TReplyParams& params) const {
    EVLOG_MSG << EVLOG_ERROR << CurrentExceptionMessage();
    if (ANTIROBOT_DAEMON_CONFIG.DebugOutput) {
        Cerr << CurrentExceptionMessage() << Endl;
    }
    Env.IncreaseReplyFails();
    params.Output << TResponse::ToUser(HTTP_INTERNAL_SERVER_ERROR);
}

} // namespace NAntiRobot
