#pragma once

#include "request_context.h"

#include <antirobot/lib/antirobot_response.h>

#include <library/cpp/threading/future/future.h>

namespace NAntiRobot {
    class TServerErrorHandler {
    public:
        NThreading::TFuture<TResponse> operator()(TRequestContext& rc);
    };
} // NAntiRobot
