#pragma once
#include "captcha_descriptor.h"
#include "request_context.h"

#include <util/generic/strbuf.h>

namespace NAntiRobot {

struct TCaptchaPageParams {
    const TRequestContext& RequestContext;
    const TCaptchaDescriptor VisibleCaptcha;
    TStringBuf FormActionPath;
    TString ReturnPath;
    TString AesKey;
    TString AesSign;
    TString CaptchaPageUrl;
    HttpCodes HttpCode;
    bool Again;
    bool CookiesEnabled;
    bool InjectGreed;

    TCaptchaPageParams(const TRequestContext& rc, const TCaptchaDescriptor& visibleCaptcha, bool again);
};

THashMap<TStringBuf, TStringBuf> GetPageDefaultParams(const TRequest& req, const TStringMap& localizedVars);

} /* namespace NAntiRobot */
