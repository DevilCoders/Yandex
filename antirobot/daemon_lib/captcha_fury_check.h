#pragma once

#include "captcha_api_check.h"
#include "tvm.h"

#include <antirobot/lib/error.h>
#include <antirobot/lib/spravka.h>
#include <antirobot/daemon_lib/request_params.h>

#include <library/cpp/threading/future/future.h>

#include <util/datetime/base.h>
#include <util/generic/strbuf.h>

namespace NAntiRobot {
    enum class EFuryCategory {
        CaptchaRobot /* "captcha_robot" */,
        DegradationWeb /* "degradation_web" */,
        DegradationMarket /* "degradation_market" */,
        DegradationUslugi /* "degradation_uslugi" */,
        DegradationAutoru /* "degradation_autoru" */,
        Count
    };


    struct TCaptchaFuryResult {
        bool CheckOk = true;
        TSpravka::TDegradation Degradation;
    };

    struct TFuryOptions {
        bool Enabled;
        THostAddr Host;
        TString Protocol;
        TDuration Timeout;
        TString TVMServiceTicket;
    };

    TErrorOr<TVector<EFuryCategory>> ParseFuryResult(const TString& response);

    NThreading::TFuture<TErrorOr<TCaptchaFuryResult>> GetCaptchaFuryResultAsync(
        const TCaptchaKey& key,
        const TCaptchaApiResult& captchaApiResult,
        const TRequest& request,
        const TFuryOptions& options,
        TCaptchaSettingsPtr settings,
        TTimeStats& timeStat
    );
} // namespace NAntiRobot
