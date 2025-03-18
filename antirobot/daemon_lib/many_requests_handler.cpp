#include "many_requests_handler.h"
#include "captcha_page_params.h"

#include <antirobot/captcha/localized_data.h>

namespace NAntiRobot {

namespace {

const TArchiveReader& Reader() {
    return TLocalizedData::Instance().GetArchiveReader();
}

} // anonymous namespace

TManyRequestsHandler::TManyRequestsHandler()
    : ManyRequestPage(Reader().ObjectByKey("/many_requests.html.ru")->ReadAll(), "ru")
{
}

NThreading::TFuture<TResponse> TManyRequestsHandler::operator()(const TRequestContext& rc) {
    const TRequest& req = *rc.Req;

    const auto service = ToString(req.HostType);
    const auto identType = ToString(req.Uid.Ns);

    auto params = GetPageDefaultParams(req, TLocalizedData::Instance().GetData(ManyRequestPage.ResourceKey));

    const auto response = TResponse::ToUser(HTTP_TOO_MANY_REQUESTS)
                        .AddHeader("Retry-After", "600")
                        .SetContent(ManyRequestPage.Gen(params), strByMime(MIME_HTML));


    return NThreading::MakeFuture(response);
}

} // namespace NAntiRobot
