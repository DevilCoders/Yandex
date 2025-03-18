#pragma once

#include "page_template.h"
#include "request_context.h"
#include "resource_selector.h"

#include <antirobot/lib/antirobot_response.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/mime/types/mime.h>
#include <library/cpp/threading/future/future.h>

namespace NAntiRobot {

class TBadRequestHandler {
public:
    TBadRequestHandler(HttpCodes httpCode);

    NThreading::TFuture<TResponse> operator()(const TRequestContext& rc, const TString& reason = "");

private:
    HttpCodes HttpCode;
    TResourceSelector<TPageTemplate> PageSelector;
};

class TNotFoundHandler : public TBadRequestHandler {
public:
    TNotFoundHandler() : TBadRequestHandler(HTTP_NOT_FOUND) {
    }
};

}
