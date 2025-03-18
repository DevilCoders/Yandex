#pragma once

#include "captcha_page.h"
#include "captcha_stat.h"
#include "client_type.h"
#include "request_params.h"

#include <antirobot/lib/error.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/threading/future/future.h>

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>

namespace NAntiRobot {

    struct TCaptchaToken;
    class TRequest;
    class TReturnPath;

    class TCaptchaPageRequestBad : public yexception {
    };

    struct TCaptchaImageInfo {
        TString Url;
        TString Grid;
        TString Bbox;
        TString Category;

        TCaptchaImageInfo(const TString& url, const TString& grid, const TString& bbox, const TString& category)
            : Url(url)
            , Grid(grid)
            , Bbox(bbox)
            , Category(category)
        {
        }
    };

    struct TCaptcha {
        TString Key;
        TString ImageUrl;
        TString VoiceUrl;
        TString VoiceIntroUrl;
        TString CaptchaType = {};
    };

    TString GetCaptchaTypeByTld(TStringBuf tld, bool nonBrandedPartner,
                                EHostType hostType, EClientType clientType);

    NThreading::TFuture<TErrorOr<TCaptcha>> MakeAdvancedCaptchaAsync(const TRequest& req,
                                                                     bool nonBrandedPartner,
                                                                     TCaptchaStat& captchaStat,
                                                                     TCaptchaSettingsPtr settings);

    TString CalcCaptchaPageUrl(const TRequest& req, const TReturnPath& retPath,
                              bool again, const TCaptchaToken& token, bool noSpravka);
    void CheckCaptchaPageRequestSigned(const TRequest& req);

    struct TCheckCookiesResult {
        bool CookiesEnabled;
        bool TestCookieSet;
        bool TestCookieSuccess;

        explicit TCheckCookiesResult(const TRequest& req);
    };

} // namespace NAntiRobot
