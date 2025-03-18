#include "options.h"

namespace NUdp {
    TServerOptions& TServerOptions::SetPort(ui32 port) {
        Port = port;
        return *this;
    }

    TServerOptions& TServerOptions::SetWorkers(ui32 workers) {
        Workers = workers;
        return *this;
    }

    TServerOptions& TServerOptions::SetExecutors(ui32 executors) {
        Executors = executors;
        return *this;
    }

    TServerOptions& TServerOptions::SetQueueSizeLimit(ui32 queueSizeLimit) {
        QueueSizeLimit = queueSizeLimit;
        return *this;
    }

    TServerOptions& TServerOptions::SetErrorThreads(ui32 errorThreads) {
        ErrorThreads = errorThreads;
        return *this;
    }

    TServerOptions& TServerOptions::SetErrorQueueSizeLimit(ui32 errorQueueSizeLimit) {
        ErrorQueueSizeLimit = errorQueueSizeLimit;
        return *this;
    }

    TServerOptions& TServerOptions::SetPollTimeout(TDuration pollTimeout) {
        PollTimeout = pollTimeout;
        return *this;
    }

}
