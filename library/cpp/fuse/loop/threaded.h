#pragma once

#include "counters.h"
#include "processor.h"

#include <library/cpp/logger/log.h>
#include <library/cpp/monlib/dynamic_counters/counters.h>

#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>
#include <util/thread/factory.h>
#include <util/thread/lfstack.h>

namespace NFuse {

class TThreadedFuseLoop {
public:
    TThreadedFuseLoop(
        TMountProcessorRef mount,
        size_t ioThreads,
        TLog log,
        TIntrusivePtr<NMonitoring::TDynamicCounters> counters
    );
    ~TThreadedFuseLoop();

    void Stop();
    void WaitForStop();

private:
    void DoExecute(int threadNum);

private:
    TMountProcessorRef Mount_;
    const size_t IoThreadCount_;
    TFuseLoopCounters Counters_;
    TLog Log_;

    using IThreadRef = THolder<IThreadFactory::IThread>;
    TVector<IThreadRef> PollThreads_;

    TMutex Lock_;
    THashSet<pthread_t> ThreadIds_;

    TAtomic Stop_;
};

} // namespace NVcs::NFuse
