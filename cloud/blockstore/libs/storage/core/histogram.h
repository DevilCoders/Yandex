#pragma once

#include "public.h"

#include <cloud/blockstore/libs/diagnostics/public.h>

#include <cloud/storage/core/libs/diagnostics/weighted_percentile.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

template <typename TBuckets>
struct THistogram
{
    std::array<ui64, TBuckets::BUCKETS_COUNT> Buckets;

    TVector<NMonitoring::TDynamicCounters::TCounterPtr> SolomonCounters;
    TString GroupName;

    bool ReportHistogram = false;
    bool HasSolomonCounters = false;

    THistogram()
    {
        Fill(Buckets.begin(), Buckets.end(), 0);
    }

    void Increment(ui64 value)
    {
        Increment(value, 1);
    }

    void Increment(ui64 value, ui64 count)
    {
        auto it = LowerBound(
            TBuckets::Limits.begin(),
            TBuckets::Limits.end(),
            value);
        Y_VERIFY(it != TBuckets::Limits.end());

        auto idx = std::distance(TBuckets::Limits.begin(), it);
        Buckets[idx] += count;
        if (HasSolomonCounters) {
            *SolomonCounters[idx] += count;
        }
    }

    const TVector<TBucketInfo> GetBuckets() const
    {
        TVector<TBucketInfo> result(Reserve(Buckets.size()));
        for (size_t i = 0; i < Buckets.size(); ++i) {
            result.emplace_back(
                TBuckets::Limits[i],
                Buckets[i]);
        }

        return result;
    }

    const TVector<TString> GetBucketNames(bool histogram) const
    {
        if (histogram) {
            return TBuckets::GetNames();
        } else {
            return GetDefaultPercentileNames();
        }
    }

    const TVector<double> CalculatePercentiles() const
    {
        return CalculatePercentiles(GetDefaultPercentiles());
    }

    const TVector<double> CalculatePercentiles(
        const TVector<TPercentileDesc>& percentiles) const
    {
        auto buckets = GetBuckets();

        auto result = CalculateWeightedPercentiles(
            buckets,
            percentiles);

        return result;
    }

    void Reset()
    {
        Buckets.fill(0);
    }

    void Add(const THistogram& source)
    {
        for (ui32 i = 0; i <  Buckets.size(); ++i) {
            Buckets[i] += source.Buckets[i];
        }
    }

    void AggregateWith(const THistogram& source)
    {
        Add(source);
    }

    TVector<TBucketInfo> GetPercentileBuckets() const
    {
        TVector<TBucketInfo> buckets(Reserve(Buckets.size()));
        for (ui32 idxRange = 0; idxRange < Buckets.size(); ++idxRange) {
            auto value = Buckets[idxRange];
            buckets.emplace_back(
                TBuckets::Limits[idxRange],
                value);
        }
        return buckets;
    }

    const TVector<ui64> GetSolomonHistogram() const
    {
        TVector<ui64> r(Reserve(Buckets.size()));
        for (ui32 i = 0; i < Buckets.size(); ++i) {
            r.push_back(Buckets[i]);
        }
        return r;
    }

    void Register(
        NMonitoring::TDynamicCountersPtr counters,
        bool reportHistogram = false)
    {
        Register(std::move(counters), {}, reportHistogram);
    }

    void Register(
        NMonitoring::TDynamicCountersPtr counters,
        const TString& groupName,
        bool reportHistogram = false)
    {
        ReportHistogram = reportHistogram;
        GroupName = groupName;
        if (!GroupName) {
            GroupName = "Time";
        }
        const auto& group = counters->GetSubgroup(
            ReportHistogram ? "histogram" : "percentiles",
            GroupName);
        auto buckets = GetBucketNames(ReportHistogram);
        SolomonCounters.clear();
        if (ReportHistogram) {
            for (size_t i = 0; i < buckets.size(); ++i) {
                SolomonCounters.push_back(group->GetCounter(buckets[i], true));
            }
        } else {
            for (ui32 i = 0; i < buckets.size(); ++i) {
                SolomonCounters.push_back(group->GetCounter(buckets[i]));
            }
        }
        HasSolomonCounters = true;
    }

    void Publish()
    {
        if (!HasSolomonCounters) {
            Y_VERIFY_DEBUG(0);
            return;
        }
        if (ReportHistogram) {
            auto hist = GetSolomonHistogram();

            Y_VERIFY(SolomonCounters.size() == hist.size());

            for (size_t i = 0; i < hist.size(); ++i) {
                *SolomonCounters[i] += hist[i];
            }
        } else {
            auto percentiles = CalculatePercentiles();
            Y_VERIFY(SolomonCounters.size() == percentiles.size());

            for (ui32 i = 0; i < percentiles.size(); ++i) {
                *SolomonCounters[i] = percentiles[i];
            }
        }
    }
};

}   // namespace NCloud::NBlockStore::NStorage
