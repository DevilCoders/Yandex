#pragma once

#include <util/system/datetime.h>

namespace NReverseGeocoder {
    class TStopWatch {
    public:
        void Run() {
            MicroSeconds_ = MicroSeconds();
        }

        // Return seconds with microseconds after floating point.
        double Get() {
            const ui64 CurMicroSeconds = MicroSeconds();
            return (CurMicroSeconds - MicroSeconds_) * 1e-6;
        }

    private:
        ui64 MicroSeconds_;
    };

}
