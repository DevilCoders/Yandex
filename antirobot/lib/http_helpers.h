#pragma once

#include "error.h"
#include "http_request.h"

#include <antirobot/lib/neh_requester.h>

#include <library/cpp/neh/neh.h>
#include <library/cpp/threading/future/future.h>

#include <util/datetime/base.h>

namespace NAntiRobot {
    inline bool IsSocketTimeout(const TSystemError& e) {
        // https://st.yandex-team.ru/CAPTCHA-460#1397828542000
        return e.Status() == EWOULDBLOCK || e.Status() == EAGAIN;
    }

    TString FetchHttpDataUnsafe(
        TNehRequester* nehRequester,
        const TNetworkAddress& addr,
        const THttpRequest& req,
        const TDuration& timeout,
        const TString& protocol
    );

    NThreading::TFuture<TErrorOr<NNeh::TResponseRef>> FetchHttpDataAsync(
        TNehRequester* nehRequester,
        const TNetworkAddress& addr,
        const THttpRequest& req,
        const TDuration& timeout,
        const TString& protocol
    );
}
