#pragma once

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <library/cpp/deprecated/atomic/atomic.h>

class IWatchDogHandle: public TAtomicRefCount<IWatchDogHandle> {
public:
    IWatchDogHandle() = default;

    bool Canceled() const {
        return (AtomicGet(Canceled_) == 1);
    }

    void Cancel() {
        AtomicSet(Canceled_, 1);
    }

    virtual void Check(TInstant timeNow) = 0;

    virtual ~IWatchDogHandle() {
    }

private:
    TAtomic Canceled_ = 0;
};

class IWatchDogHandleFreq: public IWatchDogHandle {
private:
    ui32 CheckIntervalSeconds;
    TInstant LastCheck;

public:
    IWatchDogHandleFreq(ui32 checkIntervalSeconds)
        : IWatchDogHandle()
        , CheckIntervalSeconds(checkIntervalSeconds)
    {
        LastCheck = Now();
    }

    virtual ~IWatchDogHandleFreq() = default;

    virtual void DoCheck(TInstant timeNow) = 0;

    void Check(TInstant timeNow) final {
        if (timeNow - LastCheck > TDuration::Seconds(CheckIntervalSeconds)) {
            DoCheck(timeNow);
            LastCheck = timeNow;
        }
    }
};

using TWatchDogHandlePtr = TIntrusivePtr<IWatchDogHandle>;
