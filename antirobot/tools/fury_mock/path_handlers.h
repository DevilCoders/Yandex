#pragma once

#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/http/server/response.h>
#include <library/cpp/http/server/http.h>

#include <functional>

typedef std::function<THttpResponse(const TRequestReplier::TReplyParams&)> TPathHandler;

THttpResponse BadRequest();

THttpResponse SetStrategyHandler(const TRequestReplier::TReplyParams&);

struct TCheckHandler {
    const TString Host;
    const ui16 Port;

    TCheckHandler(const TString& host, ui16 port);
    THttpResponse operator()(const TRequestReplier::TReplyParams&);
};

THttpResponse IncorrectXml(const TRequestReplier::TReplyParams&);
