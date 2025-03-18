#pragma once

#include "environment.h"
#include "fullreq_info.h"
#include "request_time_stats.h"

#include <antirobot/lib/antirobot_response.h>


namespace NAntiRobot {

class TReqReplier : public TRequestReplier {
public:
    TReqReplier(TEnv& env, std::function<NThreading::TFuture<TResponse>(TRequestContext&)>& handler,
                TRequestTimeStats requestTimeStats, bool failOnReadRequestTimeout)
        : Env(env)
        , ReqHandler(handler)
        , Duration(MakeMaybe<TMeasureDuration>(requestTimeStats.AnswerTimeStats))
        , WaitTime(MakeMaybe<TMeasureDuration>(requestTimeStats.WaitTimeStats))
        , FailOnReadRequestTimeout(failOnReadRequestTimeout)
        , RequestTimeStats(requestTimeStats)
    {
    }

    bool DoReply(const TReplyParams& params) override;
    void HandleTimeoutException(const TReplyParams& params) const;
    void HandleGeneralException(const TReplyParams& params) const;

    void Finalize(const TReplyParams& params) {
        params.Output.Finish();
        Duration.Clear();
    }

private:
    TEnv& Env;
    THolder<TRequestContext> RequestContext;
    std::function<NThreading::TFuture<TResponse>(TRequestContext&)>& ReqHandler;
    TMaybe<TMeasureDuration> Duration;
    TMaybe<TMeasureDuration> WaitTime;
    bool FailOnReadRequestTimeout;
    TRequestTimeStats RequestTimeStats;
    NThreading::TFuture<TResponse> Response;
    bool Delayed = false;
};

} // namespace NAntiRobot
