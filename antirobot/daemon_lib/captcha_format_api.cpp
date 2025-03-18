#include "captcha_format_api.h"

#include "captcha_gen.h"
#include "captcha_page_params.h"
#include "return_path.h"

#include <antirobot/lib/spravka.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/mime/types/mime.h>

#include <util/string/printf.h>


namespace NAntiRobot {

TApiCaptchaFormat::TApiCaptchaFormat(const TArchiveReader&) {
}

TResponse TApiCaptchaFormat::GenCaptchaResponse(TCaptchaPageParams& params) const {
    const auto& captcha = params.VisibleCaptcha;
    TString responseString;
    TStringOutput so(responseString);
    NJsonWriter::TBuf json(NJsonWriter::HEM_DONT_ESCAPE_HTML, &so);
    json.BeginObject();
    json.WriteKey("status").WriteString("failed");
    json.WriteKey("captcha");
    json.BeginObject();
    if (captcha.Key.Token.CaptchaType == ECaptchaType::SmartCheckbox) {
        json.WriteKey("key").WriteString(captcha.Key.ToString());
        json.WriteKey("type").WriteString("checkbox");
    } else if (captcha.Key.Token.CaptchaType == ECaptchaType::SmartAdvanced) {
        json.WriteKey("type").WriteString("image");
        json.WriteKey("key").WriteString(captcha.Key.ToString());
        json.WriteKey("voice").WriteString(captcha.VoiceUrl);
        json.WriteKey("voiceintro").WriteString(captcha.VoiceIntroUrl);
        json.WriteKey("image").WriteString(captcha.ImageUrl);
    }
    json.WriteKey("d").WriteString(params.AesKey);
    json.WriteKey("k").WriteString(params.AesSign);
    json.EndObject();
    json.EndObject();

    TResponse response = TResponse::ToUser(params.HttpCode).SetContent(responseString, strByMime(MIME_JSON));
    const TRequest& req = *params.RequestContext.Req;
    EnableCrossOriginResourceSharing(req, response, /* allowCredentials */ true);
    return response;
}

TResponse TApiCaptchaFormat::CaptchaSuccessReply(const TRequest& req,
                                                 const TSpravka& spravka) const {
    TString responseString;
    TStringOutput so(responseString);
    NJsonWriter::TBuf json(NJsonWriter::HEM_DONT_ESCAPE_HTML, &so);
    json.BeginObject();
    json.WriteKey("status").WriteString("ok");
    json.WriteKey("spravka").WriteString(spravka.ToString());
    json.EndObject();

    TResponse response = TResponse::ToUser(HTTP_OK).SetContent(responseString, strByMime(MIME_JSON));
    EnableCrossOriginResourceSharing(req, response, /* allowCredentials */ true);
    return response;
}

} // namespace NAntiRobot
