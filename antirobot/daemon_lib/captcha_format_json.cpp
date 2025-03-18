#include "captcha_format_json.h"

#include "captcha_gen.h"
#include "captcha_page_params.h"
#include "jsonp.h"
#include "return_path.h"

#include <antirobot/lib/spravka.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/mime/types/mime.h>

#include <util/string/printf.h>

namespace NAntiRobot {


TJsonCaptchaFormat::TJsonCaptchaFormat(const TArchiveReader& reader)
    : JsonSuccessFormat(reader.ObjectByKey("/captcha_json_success.txt")->ReadAll())
    , Page(reader.ObjectByKey("/captcha_json.txt")->ReadAll(), "ru"_sb, TPageTemplate::EEscapeMode::Json)
{
}

TResponse TJsonCaptchaFormat::GenCaptchaResponse(TCaptchaPageParams& params) const
{
    const TRequest& req = *params.RequestContext.Req;

    if (!params.Again) {
        params.CaptchaPageUrl = CalcCaptchaPageUrl(req, TReturnPath::FromRequest(req),
                                                   params.Again,
                                                   params.VisibleCaptcha.Key.Token,
                                                   false);
    }

    TString captchaContent = Page.Gen(params);

    MimeTypes mime = MIME_JSON;
    // support for JSONP format
    if (NeedJSONP(req.CgiParams)) {
        captchaContent = ToJSONP(req.CgiParams, captchaContent);
        mime = MIME_JAVASCRIPT;
    }

    TResponse response = TResponse::ToUser(params.HttpCode).SetContent(captchaContent, strByMime(mime));
    EnableCrossOriginResourceSharing(req, response, /* allowCredentials */ true);
    return response;
}

TResponse TJsonCaptchaFormat::CaptchaSuccessReply(const TRequest& req,
                                                  const TSpravka& spravka) const
{
    TString content = Sprintf(JsonSuccessFormat.data(), spravka.ToString().data());

    MimeTypes mime = MIME_JSON;
    // support for JSONP format
    if (NeedJSONP(req.CgiParams)) {
        content = ToJSONP(req.CgiParams, content);
        mime = MIME_JAVASCRIPT;
    }

    TResponse response = TResponse::ToUser(HTTP_OK).SetContent(content, strByMime(mime));
    EnableCrossOriginResourceSharing(req, response, /* allowCredentials */ true);
    return response;
}

}
