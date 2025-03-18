#include "captcha_page.h"

#include "captcha_gen.h"
#include "captcha_key.h"
#include "captcha_page_params.h"
#include "req_types.h"
#include "return_path.h"

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/generic/singleton.h>
#include <util/generic/xrange.h>
#include <util/stream/file.h>
#include <util/stream/tempbuf.h>
#include <util/string/builder.h>

#include <antirobot/lib/uri.h>

#include <utility>

namespace NAntiRobot {
    namespace {
        TString GetCurrentYear() {
            struct tm tM;
            return Strftime("%Y", TInstant::Now().GmTime(&tM));
        }
    }

    TCaptchaPage::TCaptchaPage(
        TString pageTemplate,
        TStringBuf resourceKey,
        TPageTemplate::EEscapeMode escapeMode
    )
        : PageTemplate(std::move(pageTemplate), TString(resourceKey), escapeMode)
        , LocalizedVars(TLocalizedData::Instance().GetData(resourceKey))
    {
    }

    TString TCaptchaPage::Gen(const TCaptchaPageParams& params) const {
        auto genParams = GetPageDefaultParams(*params.RequestContext.Req, LocalizedVars);

        const TString visibleCaptchaKey = params.VisibleCaptcha.Key.ToString();

        TString formAction = TString() + params.FormActionPath
            + "?key=" + EncodeUriComponent(visibleCaptchaKey)
            + "&" + TReturnPath::CGI_PARAM_NAME + "=" + EncodeUriComponent(params.ReturnPath) +
            + "&u=" + EncodeUriComponent(params.RequestContext.Req->UniqueKey);
        genParams["KEY"] = visibleCaptchaKey;
        genParams["RET_PATH"] = params.ReturnPath;
        genParams["NOESCAPE_FORM_ACTION"] = TStringBuf(formAction);
        genParams["IMG_URL"] = params.VisibleCaptcha.ImageUrl;
        genParams["CAPTCHA_VOICE_URL"] = params.VisibleCaptcha.VoiceUrl;
        genParams["CAPTCHA_VOICE_INTRO_URL"] = params.VisibleCaptcha.VoiceIntroUrl;
        genParams["YEAR"] = GetCurrentYear();
        genParams["STATUS_FAILED"] = params.Again ? TStringBuf("yes") : TStringBuf("no");
        genParams["STATUS"] = params.Again ? TStringBuf("failed") : TStringBuf();
        genParams["COOKIES_NOTENABLED"] = params.CookiesEnabled ? TStringBuf("b-hidden") : TStringBuf("b-default");
        genParams["CAPTCHA_PAGE"] = params.CaptchaPageUrl;
        genParams["ADDITIONAL_HEADER_STYLE"] =
            params.RequestContext.Req->HostType == HOST_MARKET ?
                TStringBuf(" b-head-line_service_market") : TStringBuf();

        genParams["TITLE"] = LocalizedVars.Get(TStringBuf("TITLE"));
        genParams["HEADER"] = LocalizedVars.Get(TStringBuf("HEADER"));

        genParams["AES_KEY"] = params.AesKey;
        genParams["AES_SIGN"] = params.AesSign;

        if (params.InjectGreed) {
            genParams["NOESCAPE_PGREED_SCRIPT"] =
                "<script type=\"text/javascript\" src=\"/captchapgrd\"></script>";
        }

        return PageTemplate.Gen(genParams);
    }
}
