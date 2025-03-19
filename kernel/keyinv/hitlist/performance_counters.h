#pragma once

#include <util/system/types.h>
#include <util/system/compiler.h>
#include <util/thread/singleton.h>

class TPerformanceCounters {
public:
    ui64& PosCounter() { return PosCounter_; }
    ui64& SkipCounter() { return SkipCounter_; }
    ui64& HPSkipCounter() { return HPSkipCounter_; }
    ui64& HPSwitchCounter() { return HPSwitchCounter_; }
    ui64& RestrictedPosCounter() { return RestrictedPosCounter_; }

    void Clear() {
        PosCounter_ = 0;
        SkipCounter_ = 0;
        HPSkipCounter_ = 0;
        HPSwitchCounter_ = 0;
        RestrictedPosCounter_ = 0;
    }

private:
    ui64 PosCounter_ = 0;
    ui64 SkipCounter_ = 0;
    ui64 HPSkipCounter_ = 0;
    ui64 HPSwitchCounter_ = 0;
    ui64 RestrictedPosCounter_ = 0;
};

Y_FORCE_INLINE TPerformanceCounters& ThreadLocalPerformanceCounters() {
    return *FastTlsSingleton<TPerformanceCounters>();
}
