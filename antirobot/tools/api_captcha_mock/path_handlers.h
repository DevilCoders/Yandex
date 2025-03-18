#pragma once

#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/http/server/response.h>

#include <functional>

typedef std::function<THttpResponse(const TParsedHttpFull&)> TPathHandler;

THttpResponse BadRequest();
THttpResponse PingHandler(const TParsedHttpFull&);
THttpResponse ImageHandler(const TParsedHttpFull&);
THttpResponse SetStrategyHandler(const TParsedHttpFull&);

struct TGenerateHandler {
    const TString Host;
    const ui16 Port;

    TGenerateHandler(const TString& host, ui16 port);
    THttpResponse operator()(const TParsedHttpFull& request);
};

struct TCheckHandler {
    const TString Host;
    const ui16 Port;

    TCheckHandler(const TString& host, ui16 port);
    THttpResponse operator()(const TParsedHttpFull& request);
};
