#pragma once

#include <antirobot/lib/error.h>
#include <antirobot/lib/spravka.h>
#include <antirobot/daemon_lib/request_context.h>

#include <library/cpp/threading/future/future.h>

#include <util/datetime/base.h>
#include <util/generic/strbuf.h>

namespace NAntiRobot {
    struct THostAddr;
    struct TCaptchaKey;

    struct TCaptchaCheckResult {
        bool Success = true;
        bool PreprodSuccess = true;
        bool FurySuccessChanged = false;
        bool FuryPreprodSuccessChanged = false;
        bool WasApiCaptchaError = false;
        bool WasFuryError = false;
        bool WasFuryPreprodError = false;
        TSpravka::TDegradation Degradation;
        TSpravka::TDegradation PreprodDegradation;
        TVector<TString> WarningMessages;
        TVector<TString> ErrorMessages;
    };

    NThreading::TFuture<TCaptchaCheckResult> IsCaptchaGoodAsync(const TCaptchaKey& key,
                                                                const TRequestContext& rc,
                                                                const TAntirobotDaemonConfig& cfg,
                                                                TCaptchaSettingsPtr settings);
} // namespace NAntiRobot
