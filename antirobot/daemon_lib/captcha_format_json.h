#pragma once
#include "captcha_format.h"
#include "captcha_page.h"

#include <util/generic/string.h>

class TArchiveReader;

namespace NAntiRobot {

class TJsonCaptchaFormat : public TCaptchaFormat {
public:
    explicit TJsonCaptchaFormat(const TArchiveReader& reader);
    TResponse GenCaptchaResponse(TCaptchaPageParams& params) const override;
    TResponse CaptchaSuccessReply(const TRequest& req, const TSpravka& spravka) const override;

private:
    const TString JsonSuccessFormat;
    TCaptchaPage Page;
};

} // namespace NAntiRobot
