#include "captcha_format.h"

#include "all_captcha_formats.h"
#include "request_params.h"

#include <antirobot/captcha/localized_data.h>

#include <util/generic/singleton.h>

namespace NAntiRobot {

namespace {

template<class T>
struct TCreationHelper : public T {
    TCreationHelper()
        : T(TLocalizedData::Instance().GetArchiveReader())
    {
    }
};

const TStringBuf ORIGIN_HEADER = "Origin";
const TString ALLOW_ORIGIN_HEADER = "Access-Control-Allow-Origin";
const TString ALLOW_CREDENTIALS_HEADER = "Access-Control-Allow-Credentials";

} // anonymous namespace

const TCaptchaFormat& CaptchaFormat(const TRequest& req) {
    switch (req.ClientType) {
    case CLIENT_GENERAL:
        return *Singleton<TCreationHelper<THtmlCaptchaFormat>>();
    case CLIENT_XML_PARTNER:
        return *Singleton<TCreationHelper<TXmlCaptchaFormat>>();
    case CLIENT_AJAX:
        return *Singleton<TCreationHelper<TJsonCaptchaFormat>>();
    case CLIENT_CAPTCHA_API:
        return *Singleton<TCreationHelper<TApiCaptchaFormat>>();
    default:
        Y_FAIL("Unexpected client type: %d", req.ClientType);
        return *Singleton<TCreationHelper<THtmlCaptchaFormat>>(); // Just to avoid warnings
    }
}

//
// Support for Cross-Origin Resource Sharing
// See http://www.w3.org/TR/cors/
//
void EnableCrossOriginResourceSharing(const TRequest& req, TResponse& response, bool allowCredentials) {
    if (req.Headers.Has(ORIGIN_HEADER)) {
        response.AddHeader(ALLOW_ORIGIN_HEADER, req.Headers.Get(ORIGIN_HEADER));
        if (allowCredentials) {
            response.AddHeader(ALLOW_CREDENTIALS_HEADER, TStringBuf("true"));
        }
    }
}

} // namespace NAntiRobot
