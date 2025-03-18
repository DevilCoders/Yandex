#include "time_slide.h"
#include <util/system/mutex.h>
#include <util/system/guard.h>
#include <util/generic/reserve.h>

namespace {
    struct TUnistatStorage {
        TVector<NUnistat::TIntervals> VectorsStorageForTASS;
        TMutex MutexStorageForTASS;

        TUnistatStorage()
            : VectorsStorageForTASS(::Reserve(100000))
        {
        }

        const NUnistat::TIntervals& AllocateVector(const NUnistat::TIntervals& intervalsBase) {
            ::TGuard<TMutex> g(MutexStorageForTASS);
            CHECK_WITH_LOG(VectorsStorageForTASS.size() < VectorsStorageForTASS.capacity() - 2) << "We haven't reallocate this static vector (tass stores only links)";
            VectorsStorageForTASS.emplace_back(intervalsBase);
            return VectorsStorageForTASS.back();
        }
    };
}

TTSHistogramAndTASS::TTSHistogramAndTASS(const ui32 segmentsCount, const ui32 secondsDuration, const double min, const double max, const ui32 segmentsCountHistogram, const TString& name)
    : TTimeSlidedHistogram(segmentsCount, secondsDuration, min, max, segmentsCountHistogram)
    , Name("histogram-backends-CTYPE-SERV-" + name)
    , NameForCType("histogram-backends-CTYPE-" + name)
{
    Intervals.reserve(segmentsCountHistogram + 1 + 2);
    for (ui32 i = 0; i <= segmentsCountHistogram; ++i) {
        Intervals.emplace_back(min + i * (max - min) / segmentsCountHistogram);
    }
    Init(TUnistat::Instance());
}

void TTSHistogramAndTASS::Add(const double value) {
    TTimeSlidedHistogram::Add(value);
    TUnistat::Instance().PushSignalUnsafe(Name, value);
    TUnistat::Instance().PushSignalUnsafe(NameForCType, value);
}

void TTSHistogramAndTASS::Init(TUnistat& unistat) const {
    unistat.DrillHistogramHole(Name, "dhhh", NUnistat::TPriority(50), Singleton< ::TUnistatStorage>()->AllocateVector(Intervals));
    unistat.DrillHistogramHole(NameForCType, "dhhh", NUnistat::TPriority(50), Singleton< ::TUnistatStorage>()->AllocateVector(Intervals));
}

TTimeSlidedHistogram::TTimeSlidedHistogram(const ui32 segmentsCount, const ui32 secondsDuration, const double min, const double max, const ui32 segmentsCountHistogram)
    : SecondsDuration(secondsDuration)
    , SegmentsCount(segmentsCount)
{
    CHECK_WITH_LOG(SegmentsCount > 1);
    BucketId.resize(SegmentsCount, 0);
    for (ui32 i = 0; i < SegmentsCount; ++i) {
        TimeBuckets.push_back(new TFixIntervalHistogram(min, max, segmentsCountHistogram));
    }
}

void TTimeSlidedHistogram::Clear() {
    for (auto&& bucket : TimeBuckets) {
        bucket->Clear();
    }
}

void TTimeSlidedHistogram::Add(const double value) {
    TReadGuard rg(Mutex);
    const i64 idTime = 1.0 * Now().Seconds() * SegmentsCount / SecondsDuration;
    const i64 id = idTime % TimeBuckets.size();
    if (idTime != AtomicGet(CurrentId)) {
        rg.Release();
        TWriteGuard wg(Mutex);
        if (idTime != AtomicGet(CurrentId)) {
            TimeBuckets[id]->Clear();
            AtomicSet(CurrentId, idTime);
            BucketId[id] = idTime;
        }
        TimeBuckets[id]->Add(value);
    } else {
        TimeBuckets[id]->Add(value);
    }
}

TVector<ui32> TTimeSlidedHistogram::Clone() const {
    TReadGuard rg(Mutex);
    TVector<ui32> result(TimeBuckets.front()->GetSegmentsCount() + 2, 0);
    const i64 idTime = 1.0 * Now().Seconds() * SegmentsCount / SecondsDuration;
    for (ui32 i = 0; i < TimeBuckets.size(); ++i) {
        if (idTime >= BucketId[i] && idTime - BucketId[i] <= (i64)BucketId.size()) {
            TVector<ui32> clone = TimeBuckets[i]->Clone();
            if (!result.size()) {
                result = clone;
            } else {
                for (ui32 copy = 0; copy < clone.size(); ++copy) {
                    result[copy] += clone[copy];
                }
            }
        }
    }
    return result;
}

NJson::TJsonValue TTimeSlidedHistogram::GetReport() const {
    NJson::TJsonValue result(NJson::JSON_MAP);
    TVector<ui32> values = Clone();
    for (ui32 i = 0; i < values.size(); ++i) {
        result[GetIntervalName(i)] = values[i];
    }
    return result;
}
