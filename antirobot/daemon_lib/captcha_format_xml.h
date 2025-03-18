#pragma once
#include "captcha_format.h"
#include "captcha_page.h"

namespace NAntiRobot {

class TXmlCaptchaFormat : public TCaptchaFormat {
public:
    explicit TXmlCaptchaFormat(const TArchiveReader& reader);
    TResponse GenCaptchaResponse(TCaptchaPageParams& params) const override;
    TResponse CaptchaSuccessReply(const TRequest& req, const TSpravka& spravka) const override;

private:
    TCaptchaPage Page;
};

} // namespace NAntiRobot
