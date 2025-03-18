#pragma once

#include "uid.h"

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/algorithm.h>
#include <util/system/rwlock.h>

namespace NAntiRobot {
    class TRequestRecord {
    public:
        TRequestRecord() = delete;
        TRequestRecord(TInstant firstRequestTime)
            : RequestCount(1)
            , FirstRequestTime(firstRequestTime)
        {
        }

        bool IsOutdatedRecord(TDuration rememberFor) const;
        void IncrementRequestsCount(TDuration rememberFor);
        void DecrementRequestsCount(TDuration rememberFor);
        size_t GetRequests() const;
        TDuration GetTimeAlive() const;

    private:
        size_t RequestCount;
        TInstant FirstRequestTime;
    };

    class TRpsFilter {
    public:
        TRpsFilter() = delete;
        TRpsFilter(size_t maxSizeAllowed, TDuration minSafeInterval, TDuration rememberFor)
            : MaxSizeAllowed(maxSizeAllowed)
            , MinSafeInterval(minSafeInterval)
            , RememberFor(rememberFor)
        {
        }

        void RecalcUser(const TUid& id, TInstant requestTime);
        void DecrementEntries();
        float GetRpsById(const TUid& id) const;
        size_t GetActualSize() const;

        static TInstant LastTime;

    private:
        float CalcRps(const TRequestRecord& rec) const;

        TRWMutex Mutex;
        size_t MaxSizeAllowed;
        TDuration MinSafeInterval;
        TDuration RememberFor;
        THashMap<TUid, TRequestRecord, NAntiRobot::TUidHash> Map;
    };
}
