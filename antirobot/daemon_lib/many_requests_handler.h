#pragma once

#include "page_template.h"
#include "request_context.h"

#include <antirobot/lib/antirobot_response.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/threading/future/future.h>
#include <library/cpp/mime/types/mime.h>

namespace NAntiRobot {

class TManyRequestsHandler {
public:
    TManyRequestsHandler();

    NThreading::TFuture<TResponse> operator()(const TRequestContext& rc);

private:
    TPageTemplate ManyRequestPage;
};

} // namespace NAntiRobot
