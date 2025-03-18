#pragma once

#include "processor.h"

#include <library/cpp/logger/log.h>
#include <library/cpp/monlib/dynamic_counters/counters.h>

#include <util/generic/ptr.h>

namespace NFuse {

using TMountHandle = intptr_t;

class TEpollFuseLoop {
public:
    class TRequest;

public:
    TEpollFuseLoop(
        size_t ioThreads,
        TLog log,
        TIntrusivePtr<NMonitoring::TDynamicCounters> counters
    );
    ~TEpollFuseLoop();

    TMountHandle AttachMount(TMountProcessorRef processor);
    void DetachMount(TMountHandle handle);

private:
    void Run();

private:
    class TImpl;
    THolder<TImpl> Impl_;
};

} // namespace NVcs::NFuse
