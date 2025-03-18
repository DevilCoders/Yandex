#pragma once
#include "captcha_format.h"
#include "captcha_page.h"

#include <util/generic/string.h>

class TArchiveReader;

namespace NAntiRobot {

class TApiCaptchaFormat : public TCaptchaFormat {
public:
    explicit TApiCaptchaFormat(const TArchiveReader& reader);
    TResponse GenCaptchaResponse(TCaptchaPageParams& params) const override;
    TResponse CaptchaSuccessReply(const TRequest& req, const TSpravka& spravka) const override;
};

}
