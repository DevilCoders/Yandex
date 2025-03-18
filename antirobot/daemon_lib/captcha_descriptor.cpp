#include "captcha_descriptor.h"

#include "captcha_gen.h"
#include "config_global.h"
#include "request_params.h"
#include "xml_reqs_helpers.h"

#include <library/cpp/string_utils/url/url.h>

#include <util/stream/tempbuf.h>
#include <util/generic/guid.h>


namespace NAntiRobot {

namespace {

TString ProxyCaptchaUrl(const TString& url, const TRequest& req,
                       const TCaptchaToken& token, const TStringBuf& newLocation)
{
    TStringBuf domainStr, queryStr;
    TString hostWithPort = ToString(NoXmlSearch(req.HostWithPort));
    TString returnUrl = url;
    if (req.CgiParams.Has("captcha_domain", "rambler") && TStringBuf(url).TrySplit("/image?", domainStr, queryStr)) {
        static const TString ramblerDomain = "nova.rambler.ru";
        returnUrl = "https://" + ramblerDomain + "/ext_image?" + queryStr;
        hostWithPort = ramblerDomain;
    }

    if (ANTIROBOT_DAEMON_CONFIG.ProxyCaptchaUrls) {
        return TString::Join(
            ToString(req.Scheme),
            hostWithPort,
            MakeProxiedCaptchaUrl(returnUrl, token, newLocation)
        );
    }

    return returnUrl;
}

TString AddServiceParameterToProxiedUrl(const TString& url, const EHostType service) {
    if (url.Contains('?')) {
        return TString::Join(url, "&service=", ToString(service));
    }
    return TString::Join(url, "?service=", ToString(service));
}

NThreading::TFuture<TErrorOr<TCaptchaDescriptor>> MakeAdvancedCaptchaAsyncImpl(
    const TCaptchaToken& token,
    TAtomicSharedPtr<const TRequest> req,
    bool nonBrandedPartner,
    TCaptchaStat& captchaStat,
    TCaptchaSettingsPtr settings
) {
    auto captchaFuture = MakeAdvancedCaptchaAsync(*req, nonBrandedPartner, captchaStat, settings);

    return captchaFuture.Apply([token, req = std::move(req),  settings = std::move(settings)] (const auto& future) -> TErrorOr<TCaptchaDescriptor> {
        TCaptcha captcha;
        TString client_key = settings ? settings->Getclient_key() : "";
        if (TError err = future.GetValue().PutValueTo(captcha); err.Defined()) {
            return err;
        }

        captcha.ImageUrl = AddServiceParameterToProxiedUrl(captcha.ImageUrl, req->HostType);
        captcha.ImageUrl = ProxyCaptchaUrl(captcha.ImageUrl, *req, token, TStringBuf("/captchaimg"));

        return TCaptchaDescriptor{
            TCaptchaKey{captcha.Key, token, req->Host},
            captcha.ImageUrl,
            ProxyCaptchaUrl(captcha.VoiceUrl, *req, token, TStringBuf("/captcha/voice")),
            ProxyCaptchaUrl(captcha.VoiceIntroUrl, *req, token, TStringBuf("/captcha/voiceintro")),
            captcha.CaptchaType,
        };
    });
}

NThreading::TFuture<TErrorOr<TCaptchaDescriptor>> MakeCheckboxCaptchaAsyncImpl(
    const TCaptchaToken& token, TAtomicSharedPtr<const TRequest> req
) {
    TCaptchaDescriptor result;
    TGUID guid;
    CreateGuid(&guid);
    result.Key = TCaptchaKey{GetGuidAsString(guid), token, req->Host};
    return NThreading::MakeFuture(TErrorOr<TCaptchaDescriptor>(std::move(result)));
}

}

NThreading::TFuture<TErrorOr<TCaptchaDescriptor>> MakeCaptchaAsync(
    const TCaptchaToken& token,
    TAtomicSharedPtr<const TRequest> req,
    TCaptchaStat& captchaStat,
    TCaptchaSettingsPtr settings,
    bool nonBrandedPartner
) {
    switch (token.CaptchaType) {
    case ECaptchaType::SmartAdvanced:
        return MakeAdvancedCaptchaAsyncImpl(token, req, nonBrandedPartner, captchaStat, settings);
    case ECaptchaType::SmartCheckbox:
        return MakeCheckboxCaptchaAsyncImpl(token, req);
    default:
        ythrow yexception() << "Invalid captcha type";
    }
}

}
