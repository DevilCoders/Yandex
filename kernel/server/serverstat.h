#pragma once

#include <library/cpp/unistat/unistat.h>
#include <library/cpp/http/server/response.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

class TServerStats {
    NUnistat::IHolePtr Timings = nullptr;
    NUnistat::IHolePtr ProductionTimings = nullptr;
    NUnistat::IHolePtr Code200 = nullptr;
    NUnistat::IHolePtr Code404 = nullptr;
    NUnistat::IHolePtr Code304 = nullptr;
    NUnistat::IHolePtr Code5xx = nullptr;
    NUnistat::IHolePtr CodeOther = nullptr;

    NUnistat::IHolePtr RequestQueueSize = nullptr;
    NUnistat::IHolePtr RequestQueueObjectCount = nullptr;
    NUnistat::IHolePtr RequestQueueReject = nullptr;
    NUnistat::IHolePtr FailQueueSize = nullptr;
    NUnistat::IHolePtr FailQueueObjectCount = nullptr;
    NUnistat::IHolePtr FailQueueReject = nullptr;

    class TIniter {
        TServerStats& Stats;
        const TString Prefix;
        const NUnistat::TIntervals& Intervals;

    public:
        TIniter(TServerStats& stat, TString prefix, const NUnistat::TIntervals& intervals)
            : Stats(stat)
            , Prefix(std::move(prefix))
            , Intervals(intervals)
        {
        }

        void Init(TUnistat& unistat) const;
    };

public:
    enum class EReportFormat {
        Json,
        Proto,
        ProtoHr,
        ProtoJson
    };

public:
    void Init(TString prefix, TVector<double> intervals);
    void HitTime(double time, bool productionTraffic);
    void HitRequest(HttpCodes code);
    void HitQueue(
        size_t requestQueueSize, size_t failQueueSize,
        TMaybe<size_t> requestQueueObjectCount, TMaybe<size_t> failQueueObjectCount);
    void HitFailQueueReject();
    void HitRequestQueueReject();
    void ReportMonitoringStat(IOutputStream& out, EReportFormat fmt, int level = 0) const;

private:
    TVector<double> Intervals;
};
