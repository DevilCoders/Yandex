#pragma once

#include "captcha_key.h"

#include <antirobot/lib/error.h>
#include <antirobot/lib/host_addr.h>
#include <antirobot/daemon_lib/req_types.h>
#include <antirobot/daemon_lib/captcha_stat.h>

#include <library/cpp/threading/future/future.h>

#include <util/datetime/base.h>
#include <util/generic/strbuf.h>

namespace NAntiRobot {
    struct TCaptchaApiOptions {
        THostAddr Host;
        TString Protocol;
        TDuration Timeout;
    };

    struct TCaptchaApiResult {
        bool ImageCheckOk = false;
        TVector<TString> Warnings;
        TVector<TString> Answers;
        NJson::TJsonValue SessionMetadata;
    };

    NThreading::TFuture<TErrorOr<TCaptchaApiResult>> GetCaptchaApiResultAsync(
        const TCaptchaKey& key,
        const TRequest& request,
        const TCaptchaApiOptions& options,
        TCaptchaStat& captchaStat
    );
}
