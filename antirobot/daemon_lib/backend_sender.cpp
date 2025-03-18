#include "backend_sender.h"

#include "config_global.h"
#include "eventlog_err.h"
#include "neh_requesters.h"
#include "instance_hashing.h"

#include <antirobot/lib/enum.h>
#include <antirobot/lib/http_helpers.h>

#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/iterator/mapped.h>

#include <util/generic/algorithm.h>
#include <util/generic/cast.h>
#include <util/generic/hash_set.h>
#include <util/generic/ylimits.h>
#include <util/random/random.h>
#include <util/string/builder.h>
#include <util/system/hostname.h>

#include <array>


namespace NAntiRobot {


namespace {
    TMaybe<NYP::NServiceDiscovery::TSDClient> MakeClient(
        const TString& updateFrequency, const TString& cacheDir,
        const TString& host, ui16 port
    ) {
        if (host.empty()) {
            return Nothing();
        }

        NYP::NServiceDiscovery::TSDConfig config;
        config.SetHost(host);
        config.SetPort(port);
        config.SetUpdateFrequency(updateFrequency);
        config.SetCacheDir(cacheDir);
        config.SetClientName("antirobot:" + HostName());

        return MakeMaybe<NYP::NServiceDiscovery::TSDClient>(config);
    }


    TMaybe<std::pair<TNetworkAddress, ui16>> MakeAddressFromEndpoint(
        const NYP::NServiceDiscovery::NApi::TEndpoint& endpoint,
        TMaybe<ui16> portOverride
    ) {
        ui16 port;

        if (portOverride) {
            port = *portOverride;
        } else {
            if (endpoint.port() > Max<ui16>()) {
                return {};
            }

            port = endpoint.port();
        }

        const TString* addr;

        if (!endpoint.ip6_address().empty()) {
            addr = &endpoint.ip6_address();
        } else if (!endpoint.ip4_address().empty()) {
            addr = &endpoint.ip4_address();
        } else {
            addr = &endpoint.fqdn();
        }

        try {
            return {{TNetworkAddress(*addr, port), port}};
        } catch (...) {
            return Nothing();
        }
    }


    template <typename T = TForwardingFailure>
    TForwardingFailure MakeForwardingFailure(TSourceLocation location) {
        return location + T() << "Failed to send request";
    }


    class TSkip : public TForwardingFailure {};
}


struct TBackendSender::TParsedEndpoint {
    TString Id;
    TString Host;
    TNetworkAddress Address;

    explicit TParsedEndpoint(
        TString id,
        TString host,
        const TNetworkAddress& address
    )
        : Id(std::move(id))
        , Host(std::move(host))
        , Address(address)
    {}

    static TMaybe<TParsedEndpoint> Parse(
        const NYP::NServiceDiscovery::NApi::TEndpoint& endpoint,
        TMaybe<ui16> portOverride = Nothing()
    ) {
        auto maybeAddrPort = MakeAddressFromEndpoint(endpoint, portOverride);

        if (!maybeAddrPort) {
            return Nothing();
        }

        const auto& [addr, port] = *maybeAddrPort;

        return TParsedEndpoint(
            endpoint.id(),
            TStringBuilder() << endpoint.fqdn() << ':' << port,
            addr
        );
    }
};


class TBackendSender::TData {
public:
    TData() = default;

    explicit TData(const TVector<std::pair<TString, THostAddr>>& backends) {
        auto sortedBackends = backends;

        SortBy(sortedBackends, [] (const auto& backend) {
            return backend.first;
        });

        Backends.reserve(sortedBackends.size());

        for (const auto& [id, hostAddr] : sortedBackends) {
            IdToIndex[id] = Backends.size();
            Backends.emplace_back(hostAddr.HostName, hostAddr);
        }
    }

    size_t GetNumBackends() const {
        return Backends.size();
    }

    NThreading::TFuture<TErrorOr<TString>> TrySend(
        const TIpRangeMap<size_t>& customHashingMap,
        const TAddr& addr,
        TDuration timeout,
        double maxSkipProbability,
        size_t attempt,
        THttpRequest* req
    ) const {
        const auto index = ChooseInstance(addr, attempt, Backends.size(), customHashingMap);
        const auto& backend = Backends[index];

        if (RandomNumber<double>() < Min(backend.NumFailures / 50.0, maxSkipProbability)) {
            return NThreading::MakeFuture<TErrorOr<TString>>(TError(__LOCATION__ + TSkip()));
        }

        req->SetHost(backend.Host);

        return FetchHttpDataAsync(
            &TGeneralNehRequester::Instance(),
            backend.Address,
            *req,
            timeout,
            "http"
        ).Apply([numFailures = &backend.NumFailures] (const auto& responseFuture) {
            NNeh::TResponseRef response;

            if (TError err = responseFuture.GetValue().PutValueTo(response); err.Defined()) {
                ++*numFailures;
                return TErrorOr<TString>(std::move(err));
            }

            if (!response) {
                ++*numFailures;
                return TErrorOr<TString>(MakeForwardingFailure(__LOCATION__) << " (null response)");
            }

            if (response->IsError()) {
                ++*numFailures;
                return TErrorOr<TString>(MakeForwardingFailure(__LOCATION__)
                    << " (type=" << EnumValue(response->GetErrorType())
                    << "; code=" << response->GetErrorCode()
                    << "; system code=" << response->GetSystemErrorCode()
                    << "): " << response->GetErrorText());
            }

            *numFailures = 0;

            return TErrorOr(TString(response->Data));
        });
    }

    TVector<TString> GetSequence(
        const TIpRangeMap<size_t>& customHashingMap,
        const TAddr& addr,
        size_t len
    ) const {
        TVector<TString> seq;
        seq.reserve(len);

        for (size_t i = 0; i < len; ++i) {
            const auto idx = ChooseInstance(addr, i, Backends.size(), customHashingMap);
            seq.push_back(Backends[idx].Host);
        }

        return seq;
    }

    TVector<TString> GetHosts() const {
        TVector<TString> hosts;
        hosts.reserve(Backends.size());

        for (const auto& backend : Backends) {
            hosts.push_back(backend.Host);
        }

        return hosts;
    }

    void Update(const TVector<TVector<TParsedEndpoint>>& endpointSets) {
        size_t numReceived = 0;

        for (const auto& endpoints : endpointSets) {
            for (const auto& endpoint : endpoints) {
                const auto* index = IdToIndex.FindPtr(endpoint.Id);

                if (!index) {
                    Reshard(endpointSets);
                    return;
                }

                Backends[*index] = TBackend(endpoint.Host, endpoint.Address);
                ++numReceived;
            }
        }

        if (numReceived < IdToIndex.size()) {
            Reshard(endpointSets);
        }
    }

    void Reshard(const TVector<TVector<TParsedEndpoint>>& endpointSets) {
        IdToIndex.clear();
        Backends.clear();

        TVector<TParsedEndpoint> sortedEndpoints;

        for (const auto& endpoints : endpointSets) {
            for (const auto& endpoint : endpoints) {
                sortedEndpoints.push_back(endpoint);
            }
        }

        SortBy(sortedEndpoints, [] (const auto& endpoint) {
            return endpoint.Id;
        });

        for (auto&& endpoint : sortedEndpoints) {
            IdToIndex[endpoint.Id] = Backends.size();
            Backends.emplace_back(std::move(endpoint.Host), std::move(endpoint.Address));
        }
    }

private:
    struct TBackend {
        TString Host;
        TNetworkAddress Address;
        mutable std::atomic<size_t> NumFailures = 0;

        explicit TBackend(
            TString host,
            const TNetworkAddress& address
        )
            : Host(std::move(host))
            , Address(address)
        {}

        TBackend(const TBackend& that)
            : Host(that.Host)
            , Address(that.Address)
            , NumFailures(static_cast<size_t>(that.NumFailures))
        {}

        TBackend& operator=(const TBackend& that) {
            Host = that.Host;
            Address = that.Address;
            NumFailures = static_cast<size_t>(that.NumFailures);
            return *this;
        }
    };

    THashMap<TString, size_t> IdToIndex;
    TVector<TBackend> Backends;
};


class TBackendSender::TProvider : public NYP::NServiceDiscovery::IEndpointSetProvider {
public:
    explicit TProvider(TBackendSender* backendSender, size_t index)
        : BackendSender(backendSender)
        , Index(index)
    {}

    void Update(const NYP::NServiceDiscovery::TEndpointSetEx& endpointSet) override {
        BackendSender->Update(endpointSet, Index);
    }

private:
    TBackendSender* BackendSender;
    size_t Index;
};


void TBackendSender::TStats::UpdateOnSuccess(size_t attempt) {
    ++Counters.Get(ECounter::ForwardedRequests);

    std::array<std::atomic<size_t>*, 3> tryCounters = {
        &Counters.Get(ECounter::ForwardedRequests1),
        &Counters.Get(ECounter::ForwardedRequests2),
        &Counters.Get(ECounter::ForwardedRequests3)
    };

    if (attempt < tryCounters.size()) {
        ++*tryCounters[attempt];
    }
}

void TBackendSender::TStats::LogException() {
    ++Counters.Get(ECounter::ForwardExceptions);
    EVLOG_MSG << EVLOG_ERROR << CurrentExceptionMessage();
}

void TBackendSender::TStats::Print(TStatsWriter* out) const {
    out->WriteHistogram("discovery_status", static_cast<double>(Status));
    Counters.Print(*out);
    ForwardRequestTimeSingle.PrintStats(*out);
    ForwardRequestTimeWithRetries.PrintStats(*out);
}


TBackendSender::TSendContext::TSendContext(
    const TIpRangeMap<size_t>* customHashingMap,
    TAddr addr,
    TDuration timeout,
    double maxSkipProbability,
    NThreading::TRcuAccessor<TData>::TReference data,
    TAtomicSharedPtr<TStats> stats,
    THttpRequest request
)
    : CustomHashingMap(customHashingMap)
    , Addr(std::move(addr))
    , Timeout(timeout)
    , MaxSkipProbability(maxSkipProbability)
    , Data(std::move(data))
    , Stats(std::move(stats))
    , Request(std::move(request))
{}

TBackendSender::TSendContext::TSendContext(const TSendContext& that) = default;

TBackendSender::TSendContext::TSendContext(TSendContext&& that) noexcept = default;

TBackendSender::TSendContext::~TSendContext() = default;

NThreading::TFuture<TMaybe<TString>> TBackendSender::TSendContext::Send(size_t attempt) {
    if (attempt >= Data->GetNumBackends()) {
        Stats->ForwardRequestTimeWithRetries.AddTimeSince(StartTime);
        Stats->Counters.Inc(ECounter::ForwardFailures);
        return NThreading::MakeFuture<TMaybe<TString>>(Nothing());
    }

    try {
        const auto currentStartTime = TInstant::Now();
        const NThreading::TFuture<TErrorOr<TString>>& responseFuture = Data->TrySend(
            *CustomHashingMap, Addr,
            Timeout, MaxSkipProbability,
            attempt,
            &Request
        );

        return responseFuture.Apply([
            context = std::move(*this),
            attempt,
            currentStartTime
        ] (const NThreading::TFuture<TErrorOr<TString>>& future) mutable {
            auto& stats = *context.Stats;
            stats.ForwardRequestTimeSingle.AddTimeSince(currentStartTime);


            if (future.HasException()) {
                stats.LogException();
                return context.Send(attempt + 1);
            }

            TString response;
            if (TError err = future.GetValue().PutValueTo(response); err.Defined()) {
                stats.LogException();
                if (err.Is<TNehQueueOverflowException>()) {
                    // No need to put request into already overflown queue
                    stats.ForwardRequestTimeWithRetries.AddTimeSince(context.StartTime);
                    return NThreading::MakeFuture<TMaybe<TString>>(Nothing());
                }

                return context.Send(attempt + 1);
            }

            stats.UpdateOnSuccess(attempt);
            stats.ForwardRequestTimeWithRetries.AddTimeSince(context.StartTime);

            return NThreading::MakeFuture<TMaybe<TString>>(std::move(response));
        });

    } catch (...) {
        Stats->LogException();
        return Send(attempt + 1);
    }
}


TBackendSender::TBackendSender(
    const TString& updateFrequency, const TString& cacheDir,
    TDuration timeout, double maxSkipProbability,
    const TString& host, ui16 port,
    const TVector<TAntirobotDaemonConfig::TEndpointSetDescription>& endpointSetDescriptions,
    const TVector<std::pair<TString, THostAddr>>& explicitBackends
)
    : Timeout(timeout)
    , MaxSkipProbability(maxSkipProbability)
    , DiscoveryClient(MakeClient(updateFrequency, cacheDir, host, port))
    , Data(TData(explicitBackends))
    , ExplicitData(Data.Get())
    , Stats(MakeAtomicShared<TStats>())
{
    if (!DiscoveryClient) {
        UpdateEvent.Signal();
        return;
    }

    Ports.reserve(endpointSetDescriptions.size());
    EndpointSets.resize(endpointSetDescriptions.size());
    UpdateCounters.resize(endpointSetDescriptions.size());

    for (const auto& [i, description] : Enumerate(endpointSetDescriptions)) {
        Ports.push_back(description.Port);

        const NYP::NServiceDiscovery::TEndpointSetKey key(description.Cluster, description.Id);
        DiscoveryClient->RegisterEndpointSet<TProvider>(key, this, i);
    }
}

TBackendSender::~TBackendSender() = default;

void TBackendSender::StartDiscovery() {
    if (DiscoveryClient) {
        DiscoveryClient->Start();
    }
}

NThreading::TFuture<TErrorOr<TString>> TBackendSender::Send(
    const TIpRangeMap<size_t>* customHashingMap,
    TAddr addr,
    THttpRequest req,
    bool stopDiscovery
) const {
    return TSendContext(
        customHashingMap,
        std::move(addr),
        Timeout,
        MaxSkipProbability,
        GetData(stopDiscovery),
        Stats,
        std::move(req)
    ).Send().Apply([this](const NThreading::TFuture<TMaybe<TString>>& future) -> TErrorOr<TString> {
        try {
            auto response = future.GetValue();
            if (response.Defined()) {
                return TString(response.GetRef());
            }
            return TError(__LOCATION__ + TForwardingFailure());
        } catch (yexception& ex) {
            Stats->LogException();
            return TError(ex);
        } catch (std::exception& ex) {
            Stats->LogException();
            return TError(ex);
        } catch (...) {
            Stats->LogException();
            return TError(__LOCATION__ + yexception() << CurrentExceptionMessage());
        }

    });
}

TVector<TString> TBackendSender::GetSequence(
    const TIpRangeMap<size_t>& customHashingMap,
    const TAddr& addr,
    size_t len,
    bool stopDiscovery
) const {
    return GetData(stopDiscovery)->GetSequence(customHashingMap, addr, len);
}

TVector<TString> TBackendSender::GetHosts() const {
    return Data.Get()->GetHosts();
}

void TBackendSender::PrintStats(TStatsWriter* out) const {
    Stats->Print(out);
}

NThreading::TRcuAccessor<TBackendSender::TData>::TReference TBackendSender::GetData(
    bool stopDiscovery
) const {
    return stopDiscovery ? ExplicitData : Data.Get();
}

void TBackendSender::Update(const NYP::NServiceDiscovery::TEndpointSetEx& endpointSet, size_t index) {
    const auto guard = Guard(EndpointSetMutex);
    const auto port = Ports[index];

    EndpointSets[index].clear();

    for (const auto& endpoint : endpointSet.endpoints()) {
        auto parsedEndpoint = TParsedEndpoint::Parse(endpoint, port);

        if (!parsedEndpoint) {
            continue;
        }

        EndpointSets[index].push_back(std::move(*parsedEndpoint));
    }

    ++UpdateCounters[index];

    Stats->Counters.Get(ECounter::DiscoveryUpdates) =
        *MinElement(UpdateCounters.cbegin(), UpdateCounters.cend());

    if (Stats->Counters.Get(ECounter::DiscoveryUpdates) > 0) {
        auto data = *Data.Get();
        data.Update(EndpointSets);
        Data.Set(std::move(data));

        Stats->Status = true;
        UpdateEvent.Signal();
    }
}


} // namespace NAntiRobot
