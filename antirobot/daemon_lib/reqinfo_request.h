#pragma once

#include <antirobot/lib/addr.h>
#include <antirobot/lib/antirobot_response.h>
#include <antirobot/lib/http_request.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/string.h>

namespace NAntiRobot {

extern const TString COMMAND_GETREQINFO;

struct TRequestContext;
THttpRequest CreateReqInfoRequest(const TString& host, const TString& rawreq);
NThreading::TFuture<TResponse> HandleReqInfoRequest(TRequestContext& rc);

} /* namespace NAntiRobot */
