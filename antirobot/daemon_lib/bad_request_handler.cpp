#include "bad_request_handler.h"
#include "environment.h"
#include "captcha_page_params.h"

#include <antirobot/captcha/localized_data.h>

namespace NAntiRobot {

TBadRequestHandler::TBadRequestHandler(HttpCodes httpCode)
    : HttpCode(httpCode)
    , PageSelector(TLocalizedData::Instance().GetArchiveReader(), "/bad_request.html.", [] (TStringBuf key, TString data) {
        return TPageTemplate(std::move(data), TString(key));
    })
{
}

NThreading::TFuture<TResponse> TBadRequestHandler::operator()(const TRequestContext& rc, const TString& reason) {
    const TRequest& req = *rc.Req;
    rc.Env.Log400Event(req, HttpCode, reason);//TODO: pass code

    const auto httpCode = ToString(static_cast<int>(HttpCode));

    const auto& page = PageSelector.Select(req);
    auto params = GetPageDefaultParams(req, TLocalizedData::Instance().GetData(page.ResourceKey));
    params["HTTP_CODE"] = httpCode;

    const auto response = TResponse::ToUser(HttpCode).SetContent(page.Gen(params), strByMime(MIME_HTML));

    return NThreading::MakeFuture(response);
}

} // namespace NAntiRobot
