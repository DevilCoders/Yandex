#pragma once

#include <antirobot/lib/addr.h>
#include <antirobot/lib/antirobot_response.h>
#include <antirobot/lib/http_request.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/string.h>

class TCgiParameters;
class THttpHeaders;

namespace NAntiRobot {

struct TRequestContext;

extern const TString COMMAND_GET_HOST_NAME;

THttpRequest CreateGetHostNameRequest(const TString& host, const TString& requestedIP,
                                      int processorCount);

NThreading::TFuture<TResponse> HandleGetHostNameRequest(TRequestContext& rc);

} /* namespace NAntiRobot */
