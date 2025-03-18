#include "captcha_format_html.h"

#include "captcha_page_params.h"
#include "captcha_page.h"
#include "environment.h"
#include "host_ops.h"
#include "request_classifier.h"
#include "resource_selector.h"
#include "return_path.h"

#include <antirobot/lib/keyring.h>
#include <antirobot/lib/spravka.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/mime/types/mime.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/uilangdetect/bytld.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>

#include <array>
#include <utility>


namespace NAntiRobot {


class THtmlCaptchaFormat::TImpl {
public:
    explicit TImpl(const TArchiveReader& reader)
        : StaticVersion(ANTIROBOT_DAEMON_CONFIG.StaticFilesVersion <= 0
            ? TLocalizedData::Instance().GetAntirobotVersionedFilesVersions().back()
            : ANTIROBOT_DAEMON_CONFIG.StaticFilesVersion
        )
        , CheckboxPageSelector(reader, Sprintf("/%d-captcha_checkbox.html.", StaticVersion), [] (TStringBuf key, TString data) {
            return TCaptchaPage(std::move(data), key);
        })
        , AdvancedPageSelector(reader, Sprintf("/%d-captcha_advanced.html.", StaticVersion), [] (TStringBuf key, TString data) {
            return TCaptchaPage(std::move(data), key);
        })
        , CheckboxPageSelectorForIE(reader, "/1-captcha_checkbox.html.", [] (TStringBuf key, TString data) {
            return TCaptchaPage(std::move(data), key);
        })
        , AdvancedPageSelectorForIE(reader, "/1-captcha_advanced.html.", [] (TStringBuf key, TString data) {
            return TCaptchaPage(std::move(data), key);
        })
    {
    }

    const TResourceSelector<TCaptchaPage>& SelectTemplates(const TCaptchaPageParams& params) const {
        bool isIE = params.RequestContext.CacherFeatures->IsIE;
        switch (params.VisibleCaptcha.Key.Token.CaptchaType) {
        case ECaptchaType::SmartCheckbox:
            return isIE ? CheckboxPageSelectorForIE : CheckboxPageSelector;
        case ECaptchaType::SmartAdvanced:
            return isIE ? AdvancedPageSelectorForIE : AdvancedPageSelector;
        default:
            ythrow yexception() << "Invalid captcha type";
        }
    }

private:
    int StaticVersion;
    TResourceSelector<TCaptchaPage> CheckboxPageSelector;
    TResourceSelector<TCaptchaPage> AdvancedPageSelector;
    TResourceSelector<TCaptchaPage> CheckboxPageSelectorForIE;
    TResourceSelector<TCaptchaPage> AdvancedPageSelectorForIE;
};

THtmlCaptchaFormat::THtmlCaptchaFormat(const TArchiveReader& reader)
    : Impl(new TImpl(reader))
{}

THtmlCaptchaFormat::~THtmlCaptchaFormat() = default;

TResponse THtmlCaptchaFormat::GenCaptchaResponse(TCaptchaPageParams& params) const {
    const auto& pages = Impl->SelectTemplates(params);
    const auto& req = params.RequestContext.Req;
    const auto& page = pages.Select(*req);

    return TResponse::ToUser(params.HttpCode).SetContent(page.Gen(params), strByMime(MIME_HTML));
}

TResponse THtmlCaptchaFormat::CaptchaSuccessReply(
    const TRequest& req,
    const TSpravka& spravka
) const {
    TReturnPath retPath = TReturnPath::FromCgi(req.CgiParams);
    return TResponse::Redirect(retPath.GetURL()).AddHeader("Set-Cookie", spravka.AsCookie());
}

} // namespace NAntiRobot
