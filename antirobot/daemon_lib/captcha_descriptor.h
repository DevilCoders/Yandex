#pragma once

#include "captcha_gen.h"
#include "captcha_key.h"
#include "captcha_stat.h"
#include "request_params.h"

#include <util/generic/string.h>

#include <utility>

namespace NAntiRobot {

struct TCaptchaDescriptor {
public:
    TCaptchaKey Key;
    TString ImageUrl;
    TString VoiceUrl;
    TString VoiceIntroUrl;
    TString CaptchaType;
};

NThreading::TFuture<TErrorOr<TCaptchaDescriptor>> MakeCaptchaAsync(
    const TCaptchaToken& token,
    TAtomicSharedPtr<const TRequest> req,
    TCaptchaStat& captchaStat,
    TCaptchaSettingsPtr settings = TCaptchaSettingsPtr{},
    bool nonBrandedPartner = false
);

}
