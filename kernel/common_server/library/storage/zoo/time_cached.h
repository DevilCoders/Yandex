#pragma once

#include <util/datetime/base.h>
#include <util/system/guard.h>
#include <util/system/rwlock.h>

template<typename T>
class ITimeCachedValue {
public:

    virtual T DoGet() = 0;

    virtual ~ITimeCachedValue() {

    }

    ITimeCachedValue(TDuration checkInterval)
        : CheckInterval(checkInterval)
        , Value()
        , LastCheck(TInstant::MicroSeconds(0))
    {
    }

    ITimeCachedValue(TDuration checkInterval, T value)
        : CheckInterval(checkInterval)
        , Value(value)
        , LastCheck(TInstant::Now())
    {
    }

    /**
     * @brief Get current value
     *
     * @details Will update current value via DoGet if the time has come
     * @note thread-safe
     */
    T Get() {
        if (UpdateIsNeeded()) {
            TWriteGuard guard(Lock);
            if (UpdateIsNeeded()) {
                Value = DoGet();
                LastCheck = TInstant::Now();
            }
            return Value;
        }
        TReadGuard guard(Lock);
        return Value;
    }

private:
    bool UpdateIsNeeded() const {
        return LastCheck + CheckInterval < TInstant::Now();
    }

private:
    const TDuration CheckInterval;

    T Value;
    TInstant LastCheck;
    TRWMutex Lock;
};
