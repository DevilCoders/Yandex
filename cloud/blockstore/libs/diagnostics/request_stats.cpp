#include "request_stats.h"

#include "stats_helpers.h"

#include <cloud/blockstore/libs/service/request_helpers.h>

#include <cloud/storage/core/libs/diagnostics/histogram.h>
#include <cloud/storage/core/libs/diagnostics/max_calculator.h>
#include <cloud/storage/core/libs/diagnostics/request_counters.h>
#include <cloud/storage/core/libs/diagnostics/weighted_percentile.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>

#include <util/datetime/cputimer.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore {

using namespace NMonitoring;

namespace {

////////////////////////////////////////////////////////////////////////////////

class THdrRequestPercentiles
{
    using TDynamicCounterPtr = TDynamicCounters::TCounterPtr;

private:
    TVector<TDynamicCounterPtr> CountersTotal;
    TVector<TDynamicCounterPtr> CountersSmall;
    TVector<TDynamicCounterPtr> CountersLarge;
    TVector<TDynamicCounterPtr> CountersSize;

    TLatencyHistogram TotalHist;
    TLatencyHistogram SmallHist;
    TLatencyHistogram LargeHist;
    TSizeHistogram SizeHist;

public:
    void Register(
        TDynamicCounters& counters,
        const TString& request)
    {
        auto requestGroup = counters.GetSubgroup("request", request);

        auto totalTimeGroup = requestGroup->GetSubgroup("percentiles", "Time");
        Register(*totalTimeGroup, CountersTotal);

        auto smallTimeGroup = requestGroup
            ->GetSubgroup("sizeclass", "Small")
            ->GetSubgroup("percentiles", "Time");
        Register(*smallTimeGroup, CountersSmall);

        auto largeTimeGroup = requestGroup
            ->GetSubgroup("sizeclass", "Large")
            ->GetSubgroup("percentiles", "Time");
        Register(*largeTimeGroup, CountersLarge);

        auto sizeGroup = requestGroup->GetSubgroup("percentiles", "Size");
        Register(*sizeGroup, CountersSize);
    }

    void UpdateStats()
    {
        Update(CountersTotal, TotalHist);
        Update(CountersSmall, SmallHist);
        Update(CountersLarge, LargeHist);
        Update(CountersSize, SizeHist);
    }

    void AddStats(TDuration requestTime, ui32 requestBytes)
    {
        if (requestBytes < LargeRequestSize) {
            SmallHist.RecordValue(requestTime);
        } else {
            LargeHist.RecordValue(requestTime);
        }

        TotalHist.RecordValue(requestTime);
        SizeHist.RecordValue(requestBytes);
    }

private:
    void Register(
        TDynamicCounters& countersGroup,
        TVector<TDynamicCounterPtr>& counters)
    {
        const auto& percentiles = GetDefaultPercentiles();
        for (ui32 i = 0; i < percentiles.size(); ++i) {
            counters.emplace_back(countersGroup.GetCounter(percentiles[i].second));
        }
    }

    void Update(TVector<TDynamicCounterPtr>& counters, THistogramBase& histogram)
    {
        const auto& percentiles = GetDefaultPercentiles();
        for (ui32 i = 0; i < counters.size(); ++i) {
            *counters[i] = histogram.GetValueAtPercentile(percentiles[i].first * 100);
        }
        histogram.Reset();
    }
};

////////////////////////////////////////////////////////////////////////////////

class THdrPercentiles
{
private:
    THdrRequestPercentiles ReadBlocksPercentiles;
    THdrRequestPercentiles WriteBlocksPercentiles;
    THdrRequestPercentiles ZeroBlocksPercentiles;

public:
    void Register(TDynamicCounters& counters)
    {
        ReadBlocksPercentiles.Register(
            counters,
            GetBlockStoreRequestName(EBlockStoreRequest::ReadBlocks));

        WriteBlocksPercentiles.Register(
            counters,
            GetBlockStoreRequestName(EBlockStoreRequest::WriteBlocks));

        ZeroBlocksPercentiles.Register(
            counters,
            GetBlockStoreRequestName(EBlockStoreRequest::ZeroBlocks));
    }

    void UpdateStats()
    {
        ReadBlocksPercentiles.UpdateStats();
        WriteBlocksPercentiles.UpdateStats();
        ZeroBlocksPercentiles.UpdateStats();
    }

    void AddStats(
        EBlockStoreRequest requestType,
        TDuration requestTime,
        ui32 requestBytes)
    {
        GetPercentiles(requestType).AddStats(requestTime, requestBytes);
    }

private:
    THdrRequestPercentiles& GetPercentiles(EBlockStoreRequest requestType)
    {
        switch (requestType) {
            case EBlockStoreRequest::ReadBlocks:
                return ReadBlocksPercentiles;

            case EBlockStoreRequest::WriteBlocks:
                return WriteBlocksPercentiles;

            case EBlockStoreRequest::ZeroBlocks:
                return ZeroBlocksPercentiles;

            default:
                Y_FAIL();
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class TRequestStats final
    : public IRequestStats
    , public std::enable_shared_from_this<TRequestStats>
{
private:
    const TDynamicCountersPtr Counters;
    const bool IsServerSide;

    TRequestCounters Total;
    TRequestCounters TotalSSD;
    TRequestCounters TotalHDD;
    TRequestCounters TotalSSDNonrepl;
    TRequestCounters TotalSSDMirror2;
    TRequestCounters TotalSSDMirror3;
    TRequestCounters TotalSSDLocal;

    THdrPercentiles HdrTotal;
    THdrPercentiles HdrTotalSSD;
    THdrPercentiles HdrTotalHDD;
    THdrPercentiles HdrTotalSSDNonrepl;
    THdrPercentiles HdrTotalSSDMirror2;
    THdrPercentiles HdrTotalSSDMirror3;
    THdrPercentiles HdrTotalSSDLocal;

public:
    TRequestStats(
            TDynamicCountersPtr counters,
            bool isServerSide,
            ITimerPtr timer)
        : Counters(std::move(counters))
        , IsServerSide(isServerSide)
        , Total(MakeRequestCounters(timer,
            TRequestCounters::EOption::ReportHistogram))
        , TotalSSD(MakeRequestCounters(timer,
            TRequestCounters::EOption::ReportHistogram |
            TRequestCounters::EOption::OnlyReadWriteRequests))
        , TotalHDD(MakeRequestCounters(timer,
            TRequestCounters::EOption::ReportHistogram |
            TRequestCounters::EOption::OnlyReadWriteRequests))
        , TotalSSDNonrepl(MakeRequestCounters(timer,
            TRequestCounters::EOption::ReportHistogram |
            TRequestCounters::EOption::OnlyReadWriteRequests))
        , TotalSSDMirror2(MakeRequestCounters(timer,
            TRequestCounters::EOption::ReportHistogram |
            TRequestCounters::EOption::OnlyReadWriteRequests))
        , TotalSSDMirror3(MakeRequestCounters(timer,
            TRequestCounters::EOption::ReportHistogram |
            TRequestCounters::EOption::OnlyReadWriteRequests))
        , TotalSSDLocal(MakeRequestCounters(timer,
            TRequestCounters::EOption::ReportHistogram |
            TRequestCounters::EOption::OnlyReadWriteRequests))
    {
        Total.Register(*Counters);

        auto ssd = Counters->GetSubgroup("type", "ssd");
        TotalSSD.Register(*ssd);

        auto hdd = Counters->GetSubgroup("type", "hdd");
        TotalHDD.Register(*hdd);

        auto ssdNonrepl = Counters->GetSubgroup("type", "ssd_nonrepl");
        TotalSSDNonrepl.Register(*ssdNonrepl);

        auto ssdMirror2 = Counters->GetSubgroup("type", "ssd_mirror2");
        TotalSSDMirror2.Register(*ssdMirror2);

        auto ssdMirror3 = Counters->GetSubgroup("type", "ssd_mirror3");
        TotalSSDMirror3.Register(*ssdMirror3);

        auto ssdLocal = Counters->GetSubgroup("type", "ssd_local");
        TotalSSDLocal.Register(*ssdLocal);

        if (IsServerSide) {
            HdrTotal.Register(*Counters);
            HdrTotalSSD.Register(*ssd);
            HdrTotalHDD.Register(*hdd);
            HdrTotalSSDNonrepl.Register(*ssdNonrepl);
            HdrTotalSSDMirror2.Register(*ssdMirror2);
            HdrTotalSSDMirror3.Register(*ssdMirror3);
            HdrTotalSSDLocal.Register(*ssdLocal);
        }
    }

    ui64 RequestStarted(
        NCloud::NProto::EStorageMediaKind mediaKind,
        EBlockStoreRequest requestType,
        ui32 requestBytes) override
    {
        auto requestStarted = Total.RequestStarted(
            static_cast<TRequestCounters::TRequestType>(
                TranslateLocalRequestType(requestType)),
            requestBytes);

        if (IsReadWriteRequest(requestType)) {
            GetRequestCounters(mediaKind).RequestStarted(
                static_cast<TRequestCounters::TRequestType>(
                    TranslateLocalRequestType(requestType)),
                requestBytes);
        }

        return requestStarted;
    }

    TDuration RequestCompleted(
        NCloud::NProto::EStorageMediaKind mediaKind,
        EBlockStoreRequest requestType,
        ui64 requestStarted,
        TDuration postponedTime,
        ui32 requestBytes,
        EErrorKind errorKind) override
    {
        auto requestTime = Total.RequestCompleted(
            static_cast<TRequestCounters::TRequestType>(
                TranslateLocalRequestType(requestType)),
            requestStarted,
            postponedTime,
            requestBytes,
            errorKind);

        if (IsReadWriteRequest(requestType)) {
            GetRequestCounters(mediaKind).RequestCompleted(
                static_cast<TRequestCounters::TRequestType>(
                    TranslateLocalRequestType(requestType)),
                requestStarted,
                postponedTime,
                requestBytes,
                errorKind);

            if (IsServerSide) {
                HdrTotal.AddStats(
                    TranslateLocalRequestType(requestType),
                    requestTime,
                    requestBytes);

                GetHdrPercentiles(mediaKind).AddStats(
                    TranslateLocalRequestType(requestType),
                    requestTime,
                    requestBytes);
            }
        }

        return requestTime;
    }

    void AddIncompleteStats(
        NCloud::NProto::EStorageMediaKind mediaKind,
        EBlockStoreRequest requestType,
        TDuration requestTime) override
    {
        Total.AddIncompleteStats(
            static_cast<TRequestCounters::TRequestType>(
                TranslateLocalRequestType(requestType)),
            requestTime);

        if (IsReadWriteRequest(requestType)) {
            GetRequestCounters(mediaKind).AddIncompleteStats(
                static_cast<TRequestCounters::TRequestType>(
                    TranslateLocalRequestType(requestType)),
                requestTime);
        }
    }

    void AddRetryStats(
        NCloud::NProto::EStorageMediaKind mediaKind,
        EBlockStoreRequest requestType,
        EErrorKind errorKind) override
    {
        Total.AddRetryStats(
            static_cast<TRequestCounters::TRequestType>(
                TranslateLocalRequestType(requestType)),
            errorKind);

        if (IsReadWriteRequest(requestType)) {
            GetRequestCounters(mediaKind).AddRetryStats(
                static_cast<TRequestCounters::TRequestType>(
                    TranslateLocalRequestType(requestType)),
                errorKind);
        }
    }

    void RequestPostponed(EBlockStoreRequest requestType) override
    {
        Total.RequestPostponed(
            static_cast<TRequestCounters::TRequestType>(
                TranslateLocalRequestType(requestType)));
    }

    void RequestAdvanced(EBlockStoreRequest requestType) override
    {
        Total.RequestAdvanced(
            static_cast<TRequestCounters::TRequestType>(
                TranslateLocalRequestType(requestType)));
    }

    void RequestFastPathHit(EBlockStoreRequest requestType) override
    {
        Total.RequestFastPathHit(
            static_cast<TRequestCounters::TRequestType>(
                TranslateLocalRequestType(requestType)));
    }

    void UpdateStats(bool updatePercentiles) override
    {
        Total.UpdateStats(updatePercentiles);
        TotalSSD.UpdateStats(updatePercentiles);
        TotalHDD.UpdateStats(updatePercentiles);
        TotalSSDNonrepl.UpdateStats(updatePercentiles);
        TotalSSDMirror2.UpdateStats(updatePercentiles);
        TotalSSDMirror3.UpdateStats(updatePercentiles);
        TotalSSDLocal.UpdateStats(updatePercentiles);

        if (updatePercentiles && IsServerSide) {
            HdrTotal.UpdateStats();
            HdrTotalSSD.UpdateStats();
            HdrTotalHDD.UpdateStats();
            HdrTotalSSDNonrepl.UpdateStats();
            HdrTotalSSDMirror2.UpdateStats();
            HdrTotalSSDMirror3.UpdateStats();
            HdrTotalSSDLocal.UpdateStats();
        }
    }

private:
    TRequestCounters& GetRequestCounters(
        NCloud::NProto::EStorageMediaKind mediaKind)
    {
        switch (mediaKind) {
            case NCloud::NProto::STORAGE_MEDIA_SSD:
                return TotalSSD;
            case NCloud::NProto::STORAGE_MEDIA_SSD_NONREPLICATED:
                return TotalSSDNonrepl;
            case NCloud::NProto::STORAGE_MEDIA_SSD_MIRROR2:
                return TotalSSDMirror2;
            case NCloud::NProto::STORAGE_MEDIA_SSD_MIRROR3:
                return TotalSSDMirror3;
            case NCloud::NProto::STORAGE_MEDIA_SSD_LOCAL:
                return TotalSSDLocal;
            default:
                return TotalHDD;
        }
    }

    THdrPercentiles& GetHdrPercentiles(
        NCloud::NProto::EStorageMediaKind mediaKind)
    {
        switch (mediaKind) {
            case NCloud::NProto::STORAGE_MEDIA_SSD:
                return HdrTotalSSD;
            case NCloud::NProto::STORAGE_MEDIA_SSD_NONREPLICATED:
                return HdrTotalSSDNonrepl;
            case NCloud::NProto::STORAGE_MEDIA_SSD_MIRROR2:
                return HdrTotalSSDMirror2;
            case NCloud::NProto::STORAGE_MEDIA_SSD_MIRROR3:
                return HdrTotalSSDMirror3;
            case NCloud::NProto::STORAGE_MEDIA_SSD_LOCAL:
                return HdrTotalSSDLocal;
            default:
                return HdrTotalHDD;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TRequestStatsStub final
    : public IRequestStats
{
    ui64 RequestStarted(
        NCloud::NProto::EStorageMediaKind mediaKind,
        EBlockStoreRequest requestType,
        ui32 requestBytes) override
    {
        Y_UNUSED(mediaKind);
        Y_UNUSED(requestType);
        Y_UNUSED(requestBytes);
        return GetCycleCount();
    }

    TDuration RequestCompleted(
        NCloud::NProto::EStorageMediaKind mediaKind,
        EBlockStoreRequest requestType,
        ui64 requestStarted,
        TDuration postponedTime,
        ui32 requestBytes,
        EErrorKind errorKind) override
    {
        Y_UNUSED(mediaKind);
        Y_UNUSED(requestType);
        Y_UNUSED(postponedTime);
        Y_UNUSED(requestBytes);
        Y_UNUSED(errorKind);
        return CyclesToDurationSafe(GetCycleCount() - requestStarted);
    }

    void AddIncompleteStats(
        NCloud::NProto::EStorageMediaKind mediaKind,
        EBlockStoreRequest requestType,
        TDuration requestTime) override
    {
        Y_UNUSED(mediaKind);
        Y_UNUSED(requestType);
        Y_UNUSED(requestTime);
    }

    void AddRetryStats(
        NCloud::NProto::EStorageMediaKind mediaKind,
        EBlockStoreRequest requestType,
        EErrorKind errorKind) override
    {
        Y_UNUSED(mediaKind);
        Y_UNUSED(requestType);
        Y_UNUSED(errorKind);
    }

    void RequestPostponed(EBlockStoreRequest requestType) override
    {
        Y_UNUSED(requestType);
    }

    void RequestAdvanced(EBlockStoreRequest requestType) override
    {
        Y_UNUSED(requestType);
    }

    void RequestFastPathHit(EBlockStoreRequest requestType) override
    {
        Y_UNUSED(requestType);
    }

    void UpdateStats(bool updatePercentiles) override
    {
        Y_UNUSED(updatePercentiles);
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IRequestStatsPtr CreateClientRequestStats(
    TDynamicCountersPtr counters,
    ITimerPtr timer)
{
    return std::make_shared<TRequestStats>(
        std::move(counters),
        false,
        std::move(timer));
}

IRequestStatsPtr CreateServerRequestStats(
    TDynamicCountersPtr counters,
    ITimerPtr timer)
{
    return std::make_shared<TRequestStats>(
        std::move(counters),
        true,
        std::move(timer));
}

IRequestStatsPtr CreateRequestStatsStub()
{
    return std::make_shared<TRequestStatsStub>();
}

}   // namespace NCloud::NBlockStore
