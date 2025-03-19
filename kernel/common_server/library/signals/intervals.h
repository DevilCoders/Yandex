#pragma once

#include <library/cpp/unistat/unistat.h>

namespace NRTYSignals {
    static const NUnistat::TIntervals DefaultTimeIntervals = { 0, 1, 2, 3, 5, 10, 15, 20, 25, 30,
        40, 50, 60, 70, 80, 90, 100, 125, 150, 175, 200, 225, 250, 300, 350, 400,
        500, 600, 700, 800, 900, 1000, 1500, 2000, 3000, 5000, 10000 };
    static const NUnistat::TIntervals IndexationTimeIntervals = DefaultTimeIntervals;
    static const NUnistat::TIntervals SearchTimeIntervals = DefaultTimeIntervals;
}
