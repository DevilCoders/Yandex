#pragma once

#include "stat.h"
#include "time_stats.h"

#include <antirobot/lib/error.h>
#include <antirobot/lib/http_request.h>
#include <antirobot/lib/ip_map.h>
#include <antirobot/lib/stats_writer.h>

#include <infra/yp_service_discovery/libs/sdlib/client.h>

#include <library/cpp/threading/future/future.h>
#include <library/cpp/threading/rcu/rcu_accessor.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>
#include <util/network/socket.h>
#include <util/system/event.h>
#include <util/system/mutex.h>

#include <atomic>
#include <utility>


namespace NAntiRobot {


class TBackendSender {
public:
    enum class ECounter {
        DiscoveryUpdates    /* "discovery_updates" */,
        ForwardExceptions   /* "forward_request_exceptions" */,
        ForwardFailures     /* "forward_failures" */,
        ForwardedRequests   /* "forward_request_all" */,
        ForwardedRequests1  /* "forward_request_1st_try" */,
        ForwardedRequests2  /* "forward_request_2nd_try" */,
        ForwardedRequests3  /* "forward_request_3rd_try" */,
        Skips               /* "skip_count" */,
        Count
    };

public:
    explicit TBackendSender(
        const TString& updateFrequency, const TString& cacheDir,
        TDuration timeout, double maxSkipProbability,
        const TString& host, ui16 port,
        const TVector<TAntirobotDaemonConfig::TEndpointSetDescription>& endpointSetDescriptions,
        const TVector<std::pair<TString, THostAddr>>& explicitBackends
    );

    ~TBackendSender();

    void StartDiscovery();

    TManualEvent& GetUpdateEvent() {
        return UpdateEvent;
    }

    NThreading::TFuture<TErrorOr<TString>> Send(
        const TIpRangeMap<size_t>* customHashingMap,
        TAddr addr,
        THttpRequest req,
        bool stopDiscovery = false
    ) const;

    TVector<TString> GetSequence(
        const TIpRangeMap<size_t>& customHashingMap,
        const TAddr& addr,
        size_t len,
        bool stopDiscovery = false
    ) const;

    TVector<TString> GetHosts() const;

    void PrintStats(TStatsWriter* out) const;

private:
    class TData;

private:
    NThreading::TRcuAccessor<TData>::TReference GetData(bool stopDiscovery) const;
    void Update(const NYP::NServiceDiscovery::TEndpointSetEx& endpointSet, size_t index);

private:
    struct TParsedEndpoint;
    class TProvider;

    friend class TProvider;

    struct TStats {
        std::atomic<bool> Status = false;
        TCategorizedStats<std::atomic<size_t>, ECounter> Counters;
        TTimeStats ForwardRequestTimeSingle{TIME_STATS_VEC, "forward_request_single_"};
        TTimeStats ForwardRequestTimeWithRetries{TIME_STATS_VEC, "forward_request_with_retries_"};

        void UpdateOnSuccess(size_t attempt);

        // Must be called within an exception handler.
        void LogException();

        void Print(TStatsWriter* out) const;
    };

    class TSendContext {
    public:
        explicit TSendContext(
            const TIpRangeMap<size_t>* customHashingMap,
            TAddr addr,
            TDuration timeout,
            double maxSkipProbability,
            NThreading::TRcuAccessor<TData>::TReference data,
            TAtomicSharedPtr<TStats> stats,
            THttpRequest request
        );

        TSendContext(const TSendContext& that);

        TSendContext(TSendContext&& that) noexcept;

        ~TSendContext();

        NThreading::TFuture<TMaybe<TString>> Send(size_t attempt = 0);

    private:
        const TIpRangeMap<size_t>* CustomHashingMap;
        TAddr Addr;
        TDuration Timeout;
        double MaxSkipProbability = 0;
        TInstant StartTime = TInstant::Now();
        NThreading::TRcuAccessor<TData>::TReference Data;
        TAtomicSharedPtr<TStats> Stats;
        THttpRequest Request;
    };

    TDuration Timeout;
    double MaxSkipProbability = 0;
    TVector<TMaybe<ui16>> Ports;

    TMaybe<NYP::NServiceDiscovery::TSDClient> DiscoveryClient;

    TMutex EndpointSetMutex;
    TVector<TVector<TParsedEndpoint>> EndpointSets;
    TVector<size_t> UpdateCounters;

    NThreading::TRcuAccessor<TData> Data;
    NThreading::TRcuAccessor<TData>::TReference ExplicitData;

    TManualEvent UpdateEvent;

    TAtomicSharedPtr<TStats> Stats;
};


} // namespace NAntiRobot
