#pragma once

#include <library/cpp/watchdog/timeout/config/config.pb.h>

#include <library/cpp/watchdog/lib/handle.h>
#include <library/cpp/watchdog/lib/interface.h>

class TTimeoutWatchDogOptions {
public:
    TTimeoutWatchDogOptions(TDuration timeout)
        : Timeout(timeout)
    {
    }

    TTimeoutWatchDogOptions(const NTimeoutWatchDog::TTimeoutWatchDogConfig& config)
        : Timeout(TDuration::Parse(config.GetTimeout()))
        , RandomTimeoutSpread(TDuration::Parse(config.GetRandomTimeoutSpread()))
    {
    }

    TTimeoutWatchDogOptions& SetRandomTimeoutSpread(TDuration randomTimeoutSpread) {
        RandomTimeoutSpread = randomTimeoutSpread;
        return *this;
    }

public:
    TDuration Timeout;
    TDuration RandomTimeoutSpread;
};

class TTimeoutWatchDogHandle: public IWatchDogHandle {
public:
    using TCallback = std::function<void ()>;

    void Check(TInstant timeNow) override;

    static IWatchDog* Create(const TTimeoutWatchDogOptions& options, const TCallback& callback);

private:
    TTimeoutWatchDogHandle(TInstant when, const TCallback& callback)
        : When(when)
        , Callback(callback)
    {
    }

private:
    const TInstant When;
    TCallback Callback;
};
