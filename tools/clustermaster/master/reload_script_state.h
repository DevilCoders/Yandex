#pragma once

#include <util/datetime/base.h>
#include <util/system/guard.h>
#include <util/system/spinlock.h>

class TReloadScriptState {
public:
    TReloadScriptState()
        : Impl(new TImpl)
    {
    }

    bool IsInProgress() {
        TGuard<TSpinLock> guard(Lock);
        return Impl->IsInProgress();
    }

    bool IsInProgressIgnoreDeadline() {
        TGuard<TSpinLock> guard(Lock);
        return Impl->IsInProgressIgnoreDeadline();
    }

    void ReloadWasStarted(int workersCount) {
        TGuard<TSpinLock> guard(Lock);
        Impl->ReloadWasStarted(workersCount);
    }

    void ReloadWasEnded() {
        TGuard<TSpinLock> guard(Lock);
        Impl->ReloadWasEnded();
    }
private:
    struct TImpl {
        TImpl()
            : InProgress(false)
            , Deadline(TInstant::Now())
        {
        }

        bool InProgress;
        TInstant Deadline;

        void ReloadWasStarted(int workersCount) {
            InProgress = true;
            TDuration timeout = Min(TDuration::Seconds(180), Max(TDuration::Seconds(60), TDuration::MilliSeconds(workersCount * 200)));
            Deadline = TInstant::Now() + timeout;
        }

        void ReloadWasEnded() {
            InProgress = false;
        }

        bool IsInProgress() const {
            return InProgress && (TInstant::Now() < Deadline);
        }

        bool IsInProgressIgnoreDeadline() const {
            return InProgress;
        }
    };

    THolder<TImpl> Impl;
    TSpinLock Lock;
};

