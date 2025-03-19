#pragma once

#include <util/generic/ptr.h>
#include <util/system/event.h>
#include <util/generic/noncopyable.h>

#include <library/cpp/threading/future/legacy_future.h>

namespace NSearch {

class TPeriodicExecutor: TNonCopyable {
public:
    TPeriodicExecutor(
        std::function<void()> worker,
        TDuration interval,
        std::function<void()> init = {} /* shouldn't throw exceptions */);

    ~TPeriodicExecutor();

    void ExecuteNow() {
        Waiter_.Signal();
    }

    void Wait() {
        FirstRun_.Wait();
    }

    void SignalToStop() {
        AtomicSet(Destroying_, 1);
        ExecuteNow();
    }

private:
    void ThreadFunc();

private:
    TAtomic Destroying_ = 0;
    TManualEvent Waiter_;
    TManualEvent FirstRun_;
    std::function<void()> Worker_;
    std::function<void()> Init_;
    TDuration Interval_;
    NThreading::TLegacyFuture<void> Thread_;
};

}
