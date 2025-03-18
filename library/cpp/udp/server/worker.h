#pragma once

#include "callbacks.h"
#include "options.h"
#include "packet.h"

namespace NUdp::NPrivate {
    class TWorker {
    public:
        TWorker(const TServerOptions& options,
                TCallback callback,
                size_t workerId);
        ~TWorker();

        void Init();
        void Start();
        void WaitForWorkerRunning();
        void Wait();
        void Stop();
        void SetErrorCallback(TErrorCallback callback);

    private:
        class TImpl;
        THolder<TImpl> Impl_;
    };

}
