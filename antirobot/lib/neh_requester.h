#pragma once

#include "error.h"

#include <library/cpp/neh/multiclient.h>
#include <library/cpp/threading/future/future.h>

#include <util/system/thread.h>

namespace NAntiRobot {
    class TNehRequester {
    public:
        TNehRequester(size_t maxQueueSize);

        NThreading::TFuture<TErrorOr<NNeh::TResponseRef>> RequestAsync(const NNeh::TMessage& msg, TInstant deadline);

        NThreading::TFuture<TErrorOr<NNeh::TResponseRef>> RequestAsync(const NNeh::TMessage& msg, TDuration timeout) {
            return RequestAsync(msg, timeout.ToDeadLine());
        }

        ~TNehRequester();

        size_t NehQueueSize() {
            return Client->QueueSize();
        }

    private:
        static void* DispatchLoop(void* ptr);

    private:
        size_t MaxQueueSize;
        NNeh::TMultiClientPtr Client;
        TThread DispatcherThread;
    };
}
