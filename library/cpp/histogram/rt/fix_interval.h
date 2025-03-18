#pragma once

#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/generic/ptr.h>
#include <library/cpp/logger/global/global.h>
#include <util/generic/string.h>

class TFixIntervalHistogram {
private:
    TAtomic* Counters;
    const double Min;
    const double Max;
    const ui32 SegmentsCount;

public:
    TString ToString() const {
        TStringStream ss;
        for (ui32 i = 0; i < SegmentsCount + 2; ++i) {
            ss << AtomicGet(Counters[i]) << ",";
        }
        return ss.Str();
    }

    using TPtr = TAtomicSharedPtr<TFixIntervalHistogram>;

    ui32 GetSegmentsCount() const {
        return SegmentsCount;
    }

    ~TFixIntervalHistogram() {
        delete[] Counters;
    }

    TFixIntervalHistogram(const double min, const double max, const ui32 segmentsCount)
        : Min(min)
        , Max(max)
        , SegmentsCount(segmentsCount)
    {
        CHECK_WITH_LOG(max > min);
        CHECK_WITH_LOG(SegmentsCount > 0);
        Counters = new TAtomic[SegmentsCount + 2];
        Clear();
    }

    TString GetIntervalName(const ui32 index) const {
        if (index == 0) {
            return "-inf, " + Sprintf("%0*.2f", 7, Min);
        }
        if (index >= SegmentsCount + 1) {
            return Sprintf("%0*.2f", 7, Max) + ", +inf";
        }
        const double from = Min + (index - 1) * (Max - Min) / SegmentsCount;
        const double to = Min + (index) * (Max - Min) / SegmentsCount;
        return Sprintf("%0*.2f", 7, from) + ", " + Sprintf("%0*.2f", 7, to);
    }

    void Add(const double value) {
        if (value < Min) {
            AtomicIncrement(Counters[0]);
        } else if (value >= Max) {
            AtomicIncrement(Counters[SegmentsCount + 1]);
        } else {
            const ui32 id = (value - Min) / (Max - Min) * SegmentsCount;
            AtomicIncrement(Counters[id + 1]);
        }
    }

    void Clear() {
        for (ui32 i = 0; i < SegmentsCount + 2; ++i) {
            AtomicSet(Counters[i], 0);
        }
    }

    TVector<ui32> Clone() const {
        TVector<ui32> result;
        for (ui32 i = 0; i < SegmentsCount + 2; ++i) {
            result.push_back(AtomicGet(Counters[i]));
        }
        return result;
    }
};
