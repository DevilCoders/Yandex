#pragma once

#include <library/cpp/histogram/rt/time_slide.h>
#include <array>

namespace NCommonProxy {

    class TCounter {
    public:
        TCounter(const TString& name);
        ui64 GetCount() const;
        ui32 GetCount(const TDuration& period) const;

        inline void Hit() {
            Rate.Hit();
            AtomicIncrement(Processed);
        }

        void WriteInfo(NJson::TJsonValue& result) const;

    private:
        static constexpr std::array<ui32, 7> Periods = { 1, 10, 30, 60, 180, 300, 600 };
        TTimeSlidedCounter Rate;
        TAtomic Processed = 0;
        TString Name;
    };

}
