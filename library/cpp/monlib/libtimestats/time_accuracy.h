#pragma once

#include <util/datetime/base.h>
#include <util/generic/string.h>

#include <functional>

namespace NTimeStats {
    enum class ETimeAccuracy {
        MICROSECONDS,
        MILLISECONDS,
        SECONDS,
        MINUTES,
        HOURS,
        DAYS,
    };

    struct TTimeAccuracyFormatter {
        TTimeAccuracyFormatter(ETimeAccuracy accuracy);

        std::function<ui64(const TDuration&)> Duration2Num;
        TString AccuracyTitle;
    };

}
