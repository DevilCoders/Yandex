#pragma once
#include "captcha_format.h"

#include <util/generic/ptr.h>

class TArchiveReader;


namespace NAntiRobot {


class THtmlCaptchaFormat : public TCaptchaFormat {
public:
    explicit THtmlCaptchaFormat(const TArchiveReader& reader);
    ~THtmlCaptchaFormat() override;

    TResponse GenCaptchaResponse(TCaptchaPageParams& params) const override;

    /// @throw TReturnPath::TInvalidRetPathException
    TResponse CaptchaSuccessReply(const TRequest& req, const TSpravka& spravka) const override;

private:
    class TImpl;
    THolder<TImpl> Impl;
};


} // namespace NAntiRobot
