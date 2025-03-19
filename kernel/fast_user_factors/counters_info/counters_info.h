#pragma once

#include <kernel/fast_user_factors/counters/decay_counter.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/memory/pool.h>

#include <tuple>

namespace NFastUserFactors {

    template <typename TAllocator>
    void ParseRawBytes(const TStringBuf s, TVector<ui8, TAllocator>& result) noexcept;

    class TCompactQueryAggregation;

    using TCounters = TVector<TDecayCounter, TPoolAllocator>;

    template <typename K, typename V>
    using TMemoryPoolHashMap = THashMap<K, V, THash<K>, TEqualTo<K>, TPoolAllocator>;

    using TKeyToCounters = TMemoryPoolHashMap<ui32, TCounters>;
    using TStringKeyToCounters = TMemoryPoolHashMap<TString, TCounters>;

    struct TCountersInfo {
        TCountersInfo(TMemoryPool* pool) noexcept
            : QueryCounters(pool)
            , UrlHashToCounters(pool)
            , OwnerHashToCounters(pool)
        {
        }

        TCounters QueryCounters;
        TKeyToCounters UrlHashToCounters;
        TKeyToCounters OwnerHashToCounters;
        ui32 VersionId = 1;
        ui32 HashCountersVersionId = 1;

        void Clear() noexcept {
            QueryCounters.clear();
            UrlHashToCounters.clear();
            OwnerHashToCounters.clear();
            VersionId = 1;
            HashCountersVersionId = 1;
        }
    };

    float GetCounter(const TCounters* counters, const size_t index) noexcept;

    std::tuple<size_t, ui64> CompactQueryAggregation2CountersInfo(
        const TCompactQueryAggregation& qAggr,
        TCountersInfo& result,
        const ui64 timestamp,
        const std::initializer_list<TDecayCounter>& initCounters,
        TMemoryPool* memoryPool
    );

    void CompactQueryAggregation2Counters(
        const TCompactQueryAggregation& qAggr,
        TCounters& result,
        const ui64 timestamp,
        const std::initializer_list<TDecayCounter>& initCounters,
        TMemoryPool* memoryPool,
        ui32& VersionId
    );

    // Perform basic sanity check on TCompactQueryAggregation
    bool SanityCheck(const TCompactQueryAggregation& qAggr);
}
