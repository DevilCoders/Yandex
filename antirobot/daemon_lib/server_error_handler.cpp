#include "server_error_handler.h"
#include "environment.h"

namespace NAntiRobot {

NThreading::TFuture<TResponse> TServerErrorHandler::operator()(TRequestContext& rc) {
    const TRequest& req = *rc.Req;
    rc.Env.ServerErrorStats.Update(req);

    auto response = TResponse::ToBalancer(HTTP_INTERNAL_SERVER_ERROR);
    return NThreading::MakeFuture(response);
}

} // namespace NAntiRobot
