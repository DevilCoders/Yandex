#include "time_accuracy.h"

namespace NTimeStats {
    TTimeAccuracyFormatter::TTimeAccuracyFormatter(ETimeAccuracy accuracy) {
        switch (accuracy) {
            case ETimeAccuracy::MICROSECONDS: {
                Duration2Num = [](const TDuration& d) { return d.MicroSeconds(); };
                AccuracyTitle = "us";
                break;
            }
            case ETimeAccuracy::MILLISECONDS: {
                Duration2Num = [](const TDuration& d) { return d.MilliSeconds(); };
                AccuracyTitle = "ms";
                break;
            }
            case ETimeAccuracy::SECONDS: {
                Duration2Num = [](const TDuration& d) { return d.Seconds(); };
                AccuracyTitle = "s";
                break;
            }
            case ETimeAccuracy::MINUTES: {
                Duration2Num = [](const TDuration& d) { return d.Minutes(); };
                AccuracyTitle = "min";
                break;
            }
            case ETimeAccuracy::HOURS: {
                Duration2Num = [](const TDuration& d) { return d.Hours(); };
                AccuracyTitle = "h";
                break;
            }
            case ETimeAccuracy::DAYS: {
                Duration2Num = [](const TDuration& d) { return d.Days(); };
                AccuracyTitle = "d";
                break;
            }
        }
    }

}
