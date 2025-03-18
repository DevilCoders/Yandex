#include "blocked_handler.h"

#include "environment.h"
#include "host_ops.h"
#include "jsonp.h"
#include "request_params.h"
#include "return_path.h"
#include "user_reply.h"
#include "captcha_page_params.h"

#include <antirobot/captcha/localized_data.h>
#include <antirobot/lib/ar_utils.h>

#include <library/cpp/string_utils/quote/quote.h>

namespace NAntiRobot {


namespace {
    HttpCodes GetBlockCode(const TRequest& req, MimeTypes mimeType) {
        if (mimeType == MIME_JSON) {
            return HTTP_OK;
        }

        return req.ClientType  == CLIENT_GENERAL ? ANTIROBOT_DAEMON_CONFIG.JsonConfig[req.HostType].BlockCode
                                                    : HTTP_OK;
    }

    const TArchiveReader& Reader() {
        return TLocalizedData::Instance().GetArchiveReader();
    }
}

TBlockedHandler::TBlockedHandler()
    : PageSelector(Reader(), "/blocked.html.", [] (TStringBuf key, TString data) {
        return TPageTemplate(std::move(data), TString(key));
    })
    , PartnerPage(Reader().ObjectByKey("/blocked_partner.xml")->ReadAll())
    , AjaxPage(Reader().ObjectByKey("/captcha_json.txt")->ReadAll(), "ru")
    , MobileJsonSearchPage(Reader().ObjectByKey("/blocked_mjs_json.txt")->ReadAll())
{}

NThreading::TFuture<TResponse> TBlockedHandler::operator()(TRequestContext& rc) {
    const TRequest& req = *rc.Req;

    if (req.CaptchaReqType == CAPTCHAREQ_NONE) {
        rc.RobotnessHeaderValue = 1.0f;
        if (!CutRequestForwarding(rc)) {
            rc.Env.ForwardRequest(rc);
        }
    }

    TContent content = GenerateContent(rc);
    auto blockCode = GetBlockCode(req, content.ContentType);
    auto response = TResponse::ToUser(blockCode).SetContent(content.Content, strByMime(content.ContentType));
    response.AddHeader("Cache-Control", "public, max-age=120, immutable");
    return NThreading::MakeFuture(response);
}

TBlockedHandler::TContent TBlockedHandler::GenerateContent(const TRequestContext& rc) const {
    const TRequest& req = *rc.Req;

    if (req.ClientType == CLIENT_XML_PARTNER) {
        return {MIME_XML, PartnerPage};
    }

    if (req.ClientType == CLIENT_AJAX) {
        return JsonBlock(req);
    }

    const auto service = ToString(req.HostType);
    const auto identType = ToString(req.Uid.Ns);

    const auto& page = PageSelector.Select(req);
    auto params = GetPageDefaultParams(req, TLocalizedData::Instance().GetData(page.ResourceKey));

    return {MIME_HTML, page.Gen(params)};
}

TBlockedHandler::TContent TBlockedHandler::JsonBlock(const TRequest& req) const {
    // https://st.yandex-team.ru/CAPTCHA-37#1399982197000
    if (req.CanShowCaptcha()) {
        try {
            TString captchaPageUrl = TReturnPath::FromRequest(req).GetURL();

            THashMap<TStringBuf, TStringBuf> params = {
                {"IMG_URL", "http://yandex.ru/captchaimg?invalid_url"},
                {"KEY", "invalid_key"},
                {"STATUS", ""},
                {"CAPTCHA_PAGE", captchaPageUrl}
            };

            TString result = AjaxPage.Gen(params);

            if (NeedJSONP(req.CgiParams)) {
                return {MIME_JAVASCRIPT, ToJSONP(req.CgiParams, result)};
            } else {
                return {MIME_JSON, result};
            }
        } catch (const TReturnPath::TInvalidRetPathException&) {
            return {MIME_JSON, "{\"type\": blocked}"};
        }
    } else {
        return {MIME_JSON, "{\"type\": blocked}"};
    }
}


}
