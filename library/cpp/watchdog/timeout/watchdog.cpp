#include "watchdog.h"

#include <library/cpp/watchdog/lib/factory.h>

#include <util/generic/singleton.h>
#include <util/random/random.h>

using NTimeoutWatchDog::TTimeoutWatchDogConfig;

namespace {
    TDuration GetTimeout(const TTimeoutWatchDogOptions& options) {
        TDuration timeout = options.Timeout;
        if (options.RandomTimeoutSpread) {
            timeout += TDuration::MicroSeconds(RandomNumber(options.RandomTimeoutSpread.MicroSeconds()));
        }
        return timeout;
    }
}

void TTimeoutWatchDogHandle::Check(TInstant timeNow) {
    if (timeNow >= When) {
        if (Callback) {
            Callback();
        }
    }
}

IWatchDog* TTimeoutWatchDogHandle::Create(const TTimeoutWatchDogOptions& options, const TTimeoutWatchDogHandle::TCallback& callback) {
    TWatchDogHandlePtr handlePtr(new TTimeoutWatchDogHandle(GetTimeout(options).ToDeadLine(), callback));
    return Singleton<TWatchDogFactory>()->RegisterWatchDogHandle(handlePtr);
}
