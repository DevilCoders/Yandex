#include <util/system/fasttime.h>
#include <util/datetime/systime.h>

#ifdef _win_
#include <util/system/winint.h>
#include <winsock2.h>
#endif

#if defined(_darwin_) || defined(_musl_) || defined(_cygwin_)
#define TZT void
#else
#define TZT struct timezone
#endif

#if !defined(__THROW)
#define __THROW
#endif

extern "C" int gettimeofday(struct timeval* tp, TZT*) __THROW {
    if (tp) {
        const ui64 t = InterpolatedMicroSeconds();

        tp->tv_sec = t / (ui64)1000000;
        tp->tv_usec = t % (ui64)1000000;
    }

    return 0;
}
