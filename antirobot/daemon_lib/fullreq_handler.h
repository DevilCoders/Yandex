#pragma once

#include "request_context.h"
#include "environment.h"

#include <antirobot/lib/antirobot_response.h>

#include <library/cpp/threading/future/future.h>


namespace NAntiRobot {
    class TFullreqHandler {
    public:
        explicit TFullreqHandler(const std::function<NThreading::TFuture<TResponse>(TRequestContext&)>& handler)
            : Handler(handler)
        {
        }

        NThreading::TFuture<TResponse> operator()(TRequestContext& rc);

    private:
        std::function<NThreading::TFuture<TResponse>(TRequestContext&)> Handler;
    };
}
