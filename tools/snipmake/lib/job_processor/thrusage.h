#pragma once

#include <util/system/platform.h>
#include <util/datetime/base.h>

#if defined(__FreeBSD__) || defined(__linux__)
#   include <sys/time.h>
#   include <sys/resource.h>
#endif

#if defined(RUSAGE_THREAD)
inline TDuration GetThreadUTime()
{
    struct rusage ru;
    int r = getrusage(RUSAGE_THREAD, &ru);
    if (r < 0) {
        ythrow TSystemError() << "rusage failed";
    }
    return ru.ru_utime;
}
#else
inline TDuration GetThreadUTime()
{
    return TDuration();
}
#endif

class TThreadProfileTimer {
    TDuration T;
public:
    TThreadProfileTimer() {
        Reset();
    }
    TDuration Get() {
        return GetThreadUTime() - T;
    }
    TDuration Step() {
        TDuration t = GetThreadUTime();
        TDuration d = t - T;
        T = t;
        return d;
    }
    void Reset() {
        T = GetThreadUTime();
    }
};
