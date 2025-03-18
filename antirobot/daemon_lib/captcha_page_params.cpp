#include "captcha_page_params.h"

#include "captcha_gen.h"
#include "captcha_key.h"
#include "captcha_signature_data.h"
#include "config_global.h"
#include "environment.h"
#include "request_params.h"
#include "xml_reqs_helpers.h"

#include <library/cpp/string_utils/url/url.h>

namespace NAntiRobot {

/*
 * TCaptchaPageContent
 */
TCaptchaPageParams::TCaptchaPageParams(const TRequestContext& rc,
                                       const TCaptchaDescriptor& visibleCaptcha, bool again)
    : RequestContext(rc)
    , VisibleCaptcha(visibleCaptcha)
    , HttpCode(HTTP_OK)
    , Again(again)
    , CookiesEnabled(true)
    , InjectGreed(
        (ANTIROBOT_DAEMON_CONFIG.HypocrisyInject || rc.Req->HostType == HOST_CAPTCHA_GEN || ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) &&
        !rc.Env.HypocrisyBundle.Instances.empty()
    )
{
    if (!InjectGreed) {
        TCaptchaSignatureData data(rc.Req->UserAddr.ToString());
        AesKey = data.EncodedEncryptionKey;
        AesSign = data.Signature;
    }
    if (rc.ApiAutoVersionBan) {
        HttpCode = HTTP_UNAUTHORIZED;
    }
}

THashMap<TStringBuf, TStringBuf> GetPageDefaultParams(const TRequest& req, const TStringMap& localizedVars) {
    THashMap<TStringBuf, TStringBuf> params;
    params["REQ_ID"] = req.ReqId;
    params["UNIQUE_KEY"] = req.UniqueKey;
    TString serviceSuffix = "_" + ToString(req.HostType);
    for (const char* key : {"LOGO_TITLE", "LOGO", "BACKGROUND_IMAGE", "LOGO_URL"}) {
        params[key] = localizedVars.GetByService(key, serviceSuffix);
    }
    return params;
}

} /* namespace NAntiRobot */
