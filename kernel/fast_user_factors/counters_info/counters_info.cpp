#include "counters_info.h"

#include "counters_names.h"

#include <kernel/fast_user_factors/counters/decay_counter.h>
#include <kernel/fast_user_factors/factors_utils/factors.h>
#include <kernel/fast_user_factors/protos/user_sessions_stat.pb.h>

namespace NFastUserFactors {

    template <typename TAllocator>
    void ParseRawBytes(const TStringBuf s, TVector<ui8, TAllocator>& result) noexcept {
        const char* begin = s.begin();
        const char* end = s.end();
        result.reserve((end - begin) / sizeof(char));
        for (const char* it = begin; it != end; ++it) {
            result.push_back(*it);
        }
    }

    namespace {

        void ParseUrlOrOwnerCounters(
            const TCompactQueryAggregation& qAggr,
            const size_t from,
            const size_t count,
            TKeyToCounters& result,
            const ui64 timestamp,
            ui64& latestUpdate,
            const std::initializer_list<TDecayCounter>& initCounters,
            TMemoryPool* memoryPool
        ) {
            size_t totalCounters = 0;
            if (qAggr.CountersSize() != 0) {
                totalCounters = qAggr.CountersSize();
            } else if (qAggr.HasCompressedCounters()) {
                totalCounters = qAggr.GetCompressedCounters().size();
            }

            const size_t countersPerItem = totalCounters / qAggr.HashesSize();
            size_t currCountersPos = from * countersPerItem;

            TVector<ui8, TPoolAllocator> compressedCounters(memoryPool);
            if (qAggr.HasCompressedCounters()) {
                ParseRawBytes(qAggr.GetCompressedCounters(), compressedCounters);
            }

            for (size_t i = 0; i < count; ++i) {
                auto it = result.find(qAggr.GetHashes(from + i));
                if (it == result.end()) {
                    it = result.emplace(qAggr.GetHashes(from + i), TCounters(memoryPool)).first;
                }
                TCounters& counters = it->second;

                if (counters.empty()) {
                    counters = initCounters;
                }

                const auto upTo = Min(counters.size(), countersPerItem);
                for (size_t j = 0; j < upTo; ++j) {
                    const float counter = (qAggr.CountersSize() != 0)
                        ? qAggr.GetCounters(currCountersPos + j)
                        : CharToFloat(compressedCounters[currCountersPos + j]);

                    if (qAggr.TimestampsSize() != 0) { // rtmr counters case
                        counters[j].Add(qAggr.GetTimestamps(from + i), counter);
                        if (Y_LIKELY(counters[j].LastTs() < timestamp)) {
                            counters[j].Move(timestamp);
                        }
                    } else { // saas factors case
                        counters[j].Add(timestamp, counter);
                    }
                }

                latestUpdate = Max(latestUpdate, qAggr.GetTimestamps(from + i));

                currCountersPos += countersPerItem;
            }
        }

    }

    float GetCounter(const TCounters* counters, const size_t counterIndex) noexcept {
        return counters && counterIndex < counters->size()
            ? (*counters)[counterIndex].Accumulate()
            : 0.0;
    }

    std::tuple<size_t, ui64> CompactQueryAggregation2CountersInfo(
        const TCompactQueryAggregation& qAggr,
        TCountersInfo& result,
        const ui64 timestamp,
        const std::initializer_list<TDecayCounter>& initCounters,
        TMemoryPool* memoryPool
    ) {
        ui64 latestUpdate = 0;
        size_t countersDiff = 1; // debug_info
        if (result.QueryCounters.empty()) {
            result.QueryCounters = initCounters;
        }

        if (qAggr.HasQueryCountersVersionId()) {
            result.VersionId = qAggr.GetQueryCountersVersionId();
        }
        if (qAggr.QueryCountersSize() > 0) { // Deprecated, remove later. SEARCHPERS-346
            countersDiff = 0;

            const auto upTo = Min(result.QueryCounters.size(), qAggr.QueryCountersSize());
            for (size_t i = 0; i < upTo; ++i) {
                if (qAggr.HasQueryCountersTimestamp()) { // rtmr counters case
                    latestUpdate = Max(latestUpdate, qAggr.GetQueryCountersTimestamp());
                    result.QueryCounters[i].Add(qAggr.GetQueryCountersTimestamp(), qAggr.GetQueryCounters(i));
                    if (Y_LIKELY(result.QueryCounters[i].LastTs() < timestamp)) {
                        result.QueryCounters[i].Move(timestamp);
                    }
                } else { // saas counters case
                    result.QueryCounters[i].Add(timestamp, qAggr.GetQueryCounters(i));
                }
            }
        } else if (qAggr.HasCompressedQueryCounters()) {
            countersDiff = 0;

            TVector<ui8, TPoolAllocator> compressedQueryCounters(memoryPool);
            ParseRawBytes(qAggr.GetCompressedQueryCounters(), compressedQueryCounters);

            const auto upTo = Min(result.QueryCounters.size(), compressedQueryCounters.size());
            for (size_t i = 0; i < upTo; ++i) {
                if (qAggr.HasQueryCountersTimestamp()) { // rtmr counters case
                    latestUpdate = Max(latestUpdate, qAggr.GetQueryCountersTimestamp());
                    result.QueryCounters[i].Add(qAggr.GetQueryCountersTimestamp(), CharToFloat(compressedQueryCounters.at(i)));
                    if (Y_LIKELY(result.QueryCounters[i].LastTs() < timestamp)) {
                        result.QueryCounters[i].Move(timestamp);
                    }
                }
            }
        }

        countersDiff *= qAggr.HashesSize();

        if (
            qAggr.CountersSize() == 0 && !qAggr.HasCompressedCounters()
            || qAggr.HashesSize() == 0
            || qAggr.QuerySmthStatsNumsSize() != 2
        ) {
            return { countersDiff, latestUpdate };
        }

        const size_t urlsCount = qAggr.GetQuerySmthStatsNums(0);
        const size_t ownersCount = qAggr.GetQuerySmthStatsNums(1);

        if (qAggr.HasCountersVersionId()) {
            result.HashCountersVersionId = qAggr.GetCountersVersionId();
        }

        ParseUrlOrOwnerCounters(
            qAggr,
            0,
            urlsCount,
            result.UrlHashToCounters,
            timestamp,
            latestUpdate,
            initCounters,
            memoryPool
        );
        ParseUrlOrOwnerCounters(
            qAggr,
            urlsCount,
            ownersCount,
            result.OwnerHashToCounters,
            timestamp,
            latestUpdate,
            initCounters,
            memoryPool
        );
        return { countersDiff, latestUpdate };
    }

    void CompactQueryAggregation2Counters(
        const TCompactQueryAggregation& qAggr,
        TCounters& result,
        const ui64 timestamp,
        const std::initializer_list<TDecayCounter>& initCounters,
        TMemoryPool* memoryPool,
        ui32& version
    ) {
        if (result.empty()) {
            result = initCounters;
        }

        if (qAggr.HasCountersVersionId()) {
            version = qAggr.GetCountersVersionId();
        } else if (qAggr.HasQueryCountersVersionId()) {
            version = qAggr.GetQueryCountersVersionId(); // BigRT
        }

        if (qAggr.CountersSize() != 0) {
            const auto upTo = Min(result.size(), qAggr.CountersSize());
            for (size_t i = 0; i < upTo; ++i) {
                result[i].Add(timestamp, qAggr.GetCounters(i));
            }
        } else if (qAggr.HasCompressedCounters()) {
            TVector<ui8, TPoolAllocator> compressedCounters(memoryPool);
            ParseRawBytes(qAggr.GetCompressedCounters(), compressedCounters);

            const auto upTo = Min(result.size(), compressedCounters.size());
            for (size_t i = 0; i < upTo; ++i) {
                result[i].Add(timestamp, CharToFloat(compressedCounters[i]));
            }
        } else if (qAggr.HasCompressedQueryCounters()) { // BigRT
            TVector<ui8, TPoolAllocator> compressedCounters(memoryPool);
            ParseRawBytes(qAggr.GetCompressedQueryCounters(), compressedCounters);

            const auto upTo = Min(result.size(), compressedCounters.size());
            for (size_t i = 0; i < upTo; ++i) {
                result[i].Add(timestamp, CharToFloat(compressedCounters[i]));
            }
        }
    }

    bool SanityCheck(const TCompactQueryAggregation& qAggr) {
        if (qAggr.HasCompressedQueryCounters() && !(qAggr.HasQueryCountersVersionId() && qAggr.HasQueryCountersTimestamp())) {
            return false;
        }

        if (qAggr.HasCompressedCounters() && !qAggr.HasCountersVersionId()) {
            return false;
        }

        uint32_t s = 0;
        for (const auto& x: qAggr.GetQuerySmthStatsNums()) {
            s += x;
        }

        if (0 == s) {
            return true;
        }

        if (qAggr.HashesSize() != s && qAggr.Hashes64Size() != s && qAggr.StringKeysSize() != s) {
            return false;
        }

        if (qAggr.TimestampsSize() != s) {
            return false;
        }

        if (qAggr.CountersSize() % s != 0) {
            return false;
        }

        if (qAggr.HasCompressedCounters() && (qAggr.GetCompressedCounters().size() % s != 0)) {
            return false;
        }

        return true;
    }
}
