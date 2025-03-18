#pragma once

#include "fix_interval.h"

#include <library/cpp/unistat/unistat.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/histogram/rt/proto/histogram.pb.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/rwlock.h>
#include <util/system/spinlock.h>
#include <map>

class TTimeSlidedCounter: public TNonCopyable {
private:
    TAtomic* Buckets = nullptr;
    TDuration FullInterval = TDuration::Zero();
    ui32 BucketsCount = 0;
    ui64 BucketSize = 0;
    static const ui64 MaskBucketKey = Max<ui64>() - Max<ui32>();

    bool Initialized() const {
        return !!Buckets;
    }

public:
    TTimeSlidedCounter() {
    }

    bool DeserializeActualAdd(const NSaas::TCounterIntervals& info) {
        if (info.BucketSize() <= 1) {
            return false;
        }
        if (FullInterval.MilliSeconds() != info.GetInterval()) {
            return false;
        }
        if (BucketsCount != info.BucketSize() - 1) {
            return false;
        }

        BucketSize = FullInterval.NanoSeconds() / BucketsCount;
        const ui64 secondsCurrent = Now().NanoSeconds();
        const ui64 idSegmentCurrent = secondsCurrent / BucketSize;
        const ui64 idSegmentLeft = (secondsCurrent / BucketSize) - BucketsCount;
        for (ui32 i = 0; i < info.BucketSize(); ++i) {
            const auto& bucket = info.GetBucket(i);
            const ui64 segmentIdx = bucket.GetSegmentId();
            if (segmentIdx >= idSegmentLeft && segmentIdx <= idSegmentCurrent) {
                const ui32 idx = segmentIdx % (BucketsCount + 1);
                const ui64 val = AtomicGet(Buckets[idx]);
                if ((val & MaskBucketKey) == (segmentIdx << 32)) {
                    AtomicAdd(Buckets[idx], bucket.GetCounter());
                } else {
                    AtomicSet(Buckets[idx], (segmentIdx << 32) + bucket.GetCounter());
                }
            }
        }
        return true;
    }

    bool Deserialize(const NSaas::TCounterIntervals& info) {
        if (info.BucketSize() <= 1) {
            return false;
        }
        FullInterval = TDuration::MilliSeconds(info.GetInterval());
        BucketsCount = info.BucketSize() - 1;
        BucketSize = FullInterval.NanoSeconds() / BucketsCount;
        if (!!Buckets) {
            delete[] Buckets;
        }
        Buckets = new TAtomic[BucketsCount + 1];
        for (ui32 i = 0; i < info.BucketSize(); ++i) {
            const auto& bucket = info.GetBucket(i);
            AtomicSet(Buckets[i], ((ui64)bucket.GetSegmentId() << 32) + bucket.GetCounter());
        }
        return true;
    }

    bool operator==(const TTimeSlidedCounter& counter) const {
        if (FullInterval != counter.FullInterval) {
            return false;
        }
        if (BucketsCount != counter.BucketsCount) {
            return false;
        }
        if (BucketSize != counter.BucketSize) {
            return false;
        }
        for (ui32 i = 0; i < BucketsCount; ++i) {
            if (AtomicGet(Buckets[i]) != AtomicGet(counter.Buckets[i])) {
                return false;
            }
        }
        return true;
    }

    NSaas::TCounterIntervals Serialize() const {
        CHECK_WITH_LOG(Initialized());
        NSaas::TCounterIntervals result;
        result.SetInterval(FullInterval.MilliSeconds());
        const auto actor = [&result](const ui64 segment, const ui64 value) {
            auto* bucket = result.AddBucket();
            bucket->SetSegmentId(segment);
            bucket->SetCounter(value);
        };
        ForAllSegments(actor);
        return result;
    }

    ~TTimeSlidedCounter() {
        if (!!Buckets) {
            delete[] Buckets;
        }
    }

    TDuration GetInterval() const {
        CHECK_WITH_LOG(Initialized());
        return FullInterval;
    }

    ui32 GetBucketsCount() const {
        CHECK_WITH_LOG(Initialized());
        return BucketsCount;
    }

    TTimeSlidedCounter(const TDuration fullInterval, const ui32 bucketsCount)
        : FullInterval(fullInterval)
        , BucketsCount(bucketsCount)
        , BucketSize(FullInterval.NanoSeconds() / BucketsCount)
    {
        Buckets = new TAtomic[BucketsCount + 1];
        for (ui32 i = 0; i < BucketsCount + 1; ++i) {
            AtomicSet(Buckets[i], 0);
        }
    }

    ui32 Get(TInstant beginning = TInstant::Zero()) const {
        ui32 count = 0;

        const auto actor = [&count](const ui64 /*segmentId*/, const ui64 value) {
            count += value;
        };
        ForAllActualSegments(actor, beginning);

        return count;
    }

    template <class TActor>
    void ForAllActualSegments(TActor& actor, TInstant beginning = TInstant::Zero()) const {
        CHECK_WITH_LOG(Initialized());
        const ui64 idSegmentCurrent = (Now().NanoSeconds() / BucketSize) << 32;

        const ui64 idSegmentLeft = ((Now().NanoSeconds() / BucketSize) - BucketsCount) << 32;
        const ui64 idSegmentOverride = (beginning.NanoSeconds() / BucketSize) << 32;
        const ui64 idSegmentStart = std::max(idSegmentLeft, idSegmentOverride);

        for (ui32 i = 0; i < BucketsCount + 1; ++i) {
            const ui64 val = AtomicGet(Buckets[i]);
            const ui64 isSegment = val & MaskBucketKey;
            if (isSegment >= idSegmentStart && isSegment <= idSegmentCurrent) {
                actor(isSegment >> 32, val & (~MaskBucketKey));
            }
        }
    }

    template <class TActor>
    void ForAllSegments(TActor& actor) const {
        CHECK_WITH_LOG(Initialized());
        for (ui32 i = 0; i < BucketsCount + 1; ++i) {
            const ui64 val = AtomicGet(Buckets[i]);
            const ui64 isSegment = val & MaskBucketKey;
            actor(isSegment >> 32, val & (~MaskBucketKey));
        }
    }

    void Hit(const ui32 weight = 1) {
        CHECK_WITH_LOG(Initialized());
        ui64 idSegment = Now().NanoSeconds() / BucketSize;
        const ui32 idx = idSegment % (BucketsCount + 1);
        idSegment <<= 32;
        TAtomic& atomicValue = Buckets[idx];
        ui64 currentVal = AtomicGet(atomicValue);
        while ((currentVal & MaskBucketKey) != idSegment) {
            if (AtomicCas(&atomicValue, idSegment + weight, currentVal)) {
                return;
            } else {
                //                SpinLockPause();
                currentVal = AtomicGet(atomicValue);
            }
        }
        AtomicAdd(atomicValue, weight);
    }
};

class TTimeSlidedHistogram {
private:
    TVector<TFixIntervalHistogram::TPtr> TimeBuckets;
    TVector<i64> BucketId;
    const ui32 SecondsDuration;
    const ui32 SegmentsCount;
    TAtomic CurrentId = 0;
    TRWMutex Mutex;

public:
    class TGuard {
    private:
        TTimeSlidedHistogram* Histogram;
        const TInstant StartTime = Now();

    public:
        TGuard(TTimeSlidedHistogram& histogram)
            : Histogram(&histogram)
        {
        }

        ~TGuard() {
            Release();
        }

        void Release() {
            if (Histogram) {
                Histogram->Add((Now() - StartTime).Seconds());
                Histogram = nullptr;
            }
        }
    };

public:
    TTimeSlidedHistogram(const ui32 segmentsCount, const ui32 secondsDuration, const double min, const double max, const ui32 segmentsCountHistogram);
    virtual ~TTimeSlidedHistogram() {
    }

    TString GetIntervalName(const ui32 index) const {
        return TimeBuckets[0]->GetIntervalName(index);
    }
    void Clear();
    virtual void Add(const double value);
    TVector<ui32> Clone() const;
    NJson::TJsonValue GetReport() const;
};

class TTSHistogramAndTASS: public TTimeSlidedHistogram, public TNonCopyable {
private:
    NUnistat::TIntervals Intervals;
    const TString Name;
    const TString NameForCType;

public:
    TTSHistogramAndTASS(const ui32 segmentsCount, const ui32 secondsDuration, const double min, const double max, const ui32 segmentsCountHistogram, const TString& name);

    virtual void Add(const double value) override;
    void Init(TUnistat& unistat) const;
};
