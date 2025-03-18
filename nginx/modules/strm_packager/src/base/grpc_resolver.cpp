#include <nginx/modules/strm_packager/src/base/grpc_resolver.h>

#include <util/generic/map.h>
#include <util/generic/maybe.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/string/builder.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>
#include <util/system/types.h>

#include <contrib/libs/grpc/src/core/lib/address_utils/parse_address.h>
#include <contrib/libs/grpc/src/core/ext/filters/client_channel/resolver.h>
#include <contrib/libs/grpc/src/core/ext/filters/client_channel/resolver_factory.h>
#include <contrib/libs/grpc/src/core/ext/filters/client_channel/resolver_registry.h>

inline std::strong_ordering operator<=>(const ::grpc_resolved_address& a, const ::grpc_resolved_address& b) {
    if (a.len != b.len) {
        return a.len <=> b.len;
    }

    for (size_t i = 0; i < a.len; ++i) {
        if (a.addr[i] != b.addr[i]) {
            return a.addr[i] <=> b.addr[i];
        }
    }

    return std::strong_ordering::equal;
}

inline bool operator==(const ::grpc_resolved_address& a, const ::grpc_resolved_address& b) {
    return (a <=> b) == 0;
}

namespace NStrm::NPackager::NPackagerGrpcResolver {
    class TResolver;

    struct TEndpointSet {
        TMaybe<i64> Version; // ypsd timestamp actually
        TVector<::grpc_resolved_address> Addresses;
    };

    struct TEndpointSetInAllClusters {
        i64 Version = 0;
        i64 Total = 0; // number of known endpoints in all clusters
        TMap<TString, TEndpointSet> Cluster2EndpointSet;
        TSet<TResolver*> Subscribers;
        TMaybe<TInstant> FirstCheckTime; // first time this endpoint set is checked by CheckEndpointSedIdResolved
    };

    struct TData {
        TMutex Mutex;
        bool ResolverInit = false;
        size_t RunningResolversCount = 0;
        i64 Version = 0;
        TMap<TString, TEndpointSetInAllClusters> Id2Endpoints;
    };

    static TData& CacheData() {
        return *Singleton<TData>();
    }

    class TResolver: public ::grpc_core::Resolver {
    public:
        TResolver(::grpc_core::ResolverArgs args, TData& data)
            : ::grpc_core::Resolver()
            , Enabled(false)
            , Data(data)
            , UsedVersion(-1)
            , ChannelArgs_(grpc_channel_args_copy(args.args))
            , WorkSerializer_(args.work_serializer)
            , ResultHandler_(std::move(args.result_handler))
        {
            const TGuard<TMutex> guard(Data.Mutex);

            ++Data.RunningResolversCount;

            EnSet = &data.Id2Endpoints[args.uri.authority()];
            EnSet->Subscribers.insert(this);
        }

        ~TResolver() {
            grpc_channel_args_destroy(ChannelArgs_);

            const TGuard<TMutex> guard(Data.Mutex);
            --Data.RunningResolversCount;
            EnSet->Subscribers.erase(this);
        }

        void StartLocked() override {
            Enabled = true;
            Update();
        }

        void ShutdownLocked() override {
            Enabled = false;
        }

        void Update() {
            if (!Enabled) {
                return;
            }

            ::grpc_core::Resolver::Result result;

            {
                const TGuard<TMutex> guard(Data.Mutex);

                if (UsedVersion == EnSet->Version) {
                    return;
                }

                for (const auto& [_, es] : EnSet->Cluster2EndpointSet) {
                    for (const ::grpc_resolved_address& addr : es.Addresses) {
                        result.addresses.emplace_back(addr, /* grpc_channel_args* args = */ nullptr);
                    }
                }

                result.args = grpc_channel_args_copy(ChannelArgs_);

                UsedVersion = EnSet->Version;
            }

            TMaybe<::grpc_core::ExecCtx> grpcExecCtx;
            if (!::grpc_core::ExecCtx::Get()) {
                grpcExecCtx.ConstructInPlace();
            }

            WorkSerializer_->Run([this, res = std::move(result)]() {
                ResultHandler_.get()->ReturnResult(std::move(res));
            }, DEBUG_LOCATION);
        }

    private:
        bool Enabled;

        TData& Data;
        TEndpointSetInAllClusters* EnSet;

        i64 UsedVersion;

        grpc_channel_args* ChannelArgs_;
        std::shared_ptr<grpc_core::WorkSerializer> WorkSerializer_;
        std::unique_ptr<ResultHandler> ResultHandler_;
    };

    static ::grpc_resolved_address ParseAddress(const NYP::NServiceDiscovery::NApi::TEndpoint& endpoint) {
        Y_ENSURE(endpoint.ip6_address() || endpoint.ip4_address());
        Y_ENSURE(endpoint.port() >= 0 && endpoint.port() <= 65536);

        ::grpc_resolved_address addr;

        TStringBuilder hostport;

        if (endpoint.ip6_address()) {
            hostport << "[" << endpoint.ip6_address() << "]:" << endpoint.port();
            Y_ENSURE(grpc_parse_ipv6_hostport(hostport.c_str(), &addr, /*log_errors = */ true));
        } else if (endpoint.ip4_address()) {
            hostport << endpoint.ip4_address() << ":" << endpoint.port();
            Y_ENSURE(grpc_parse_ipv4_hostport(hostport.c_str(), &addr, /*log_errors = */ true));
        }

        return addr;
    }

    void CheckAddress(const NYP::NServiceDiscovery::NApi::TEndpoint& endpoint) {
        ParseAddress(endpoint);
    }

    i64 GetDataVersion() {
        const TData& data = CacheData();
        const TGuard<TMutex> guard(data.Mutex);
        return data.Version;
    }

    size_t GetRunningResolversCount() {
        const TData& data = CacheData();
        const TGuard<TMutex> guard(data.Mutex);
        return data.RunningResolversCount;
    }

    std::pair<bool, TInstant> CheckEndpointSedIdResolved(const TString& endpointSetId) {
        TData& data = CacheData();
        const TGuard<TMutex> guard(data.Mutex);
        TEndpointSetInAllClusters& enps = data.Id2Endpoints[endpointSetId];

        if (enps.FirstCheckTime.Empty()) {
            enps.FirstCheckTime = ::TInstant::Now();
        }

        return {enps.Total > 0, *enps.FirstCheckTime};
    }

    void UpdateResolve(
        const TString& id,
        const TString& cluster,
        const ::NYP::NServiceDiscovery::NApi::TRspResolveEndpoints& resp)
    {
        using ::NYP::NServiceDiscovery::NApi::EResolveStatus;

        Y_ENSURE(resp.resolve_status() == EResolveStatus::OK || resp.resolve_status() == EResolveStatus::EMPTY);

        Y_ENSURE(resp.has_endpoint_set());

        Y_ENSURE(resp.endpoint_set().endpoint_set_id() == id);

        TData& data = CacheData();
        const TGuard<TMutex> guard(data.Mutex);

        TEndpointSetInAllClusters& allensets = data.Id2Endpoints[id];
        TEndpointSet& enset = allensets.Cluster2EndpointSet[cluster];

        // dont return on equal timestamp, so updates from config can be with constant version-timestamp
        if (resp.timestamp() < enset.Version) {
            return;
        }

        TVector<::grpc_resolved_address> newAddresses;

        for (int i = 0; i < resp.endpoint_set().endpoints_size(); ++i) {
            const auto& ep = resp.endpoint_set().endpoints(i);
            if (ep.ready()) {
                newAddresses.push_back(ParseAddress(ep));
            }
        }

        ::Sort(newAddresses.begin(), newAddresses.end());

        enset.Version = resp.timestamp();

        if (std::equal(enset.Addresses.begin(), enset.Addresses.end(), newAddresses.begin(), newAddresses.end())) {
            return;
        }

        allensets.Total += newAddresses.size() - enset.Addresses.size();
        enset.Addresses = std::move(newAddresses);
        ++allensets.Version;
        ++data.Version;

        for (TResolver* sub : allensets.Subscribers) {
            sub->Update();
        }
    }

    class TResolverFactory: public ::grpc_core::ResolverFactory {
    public:
        bool IsValidUri(const grpc_core::URI& uri) const override {
            (void)uri;
            // scheme must be already checked by grpc, so nothing to do here
            return true;
        }

        ::grpc_core::OrphanablePtr<::grpc_core::Resolver> CreateResolver(::grpc_core::ResolverArgs args) const override {
            return ::grpc_core::MakeOrphanable<TResolver>(std::move(args), CacheData());
        }

        const char* scheme() const override {
            return Scheme;
        };
    };

    void Init() {
        TData& data = CacheData();
        const TGuard<TMutex> guard(data.Mutex);
        if (!data.ResolverInit) {
            ::grpc_core::ResolverRegistry::Builder::RegisterResolverFactory(::y_absl::make_unique<TResolverFactory>());
            data.ResolverInit = true;
        }
    }

    TDataProto SaveToProto() {
        using TProtoEndpointSetInAllClusters = ::NStrm::NPackager::NResolveDataProto::TEndpointSetInAllClusters;
        using TProtoEndpointSet = ::NStrm::NPackager::NResolveDataProto::TEndpointSet;

        TData& data = CacheData();
        const TGuard<TMutex> guard(data.Mutex);

        TDataProto proto;

        proto.SetVersion(data.Version);
        for (const auto& [id, esiac] : data.Id2Endpoints) {
            TProtoEndpointSetInAllClusters& protoEsiac = *proto.AddId2Endpoints();

            protoEsiac.SetEndpointSetId(id);
            protoEsiac.SetVersion(esiac.Version);

            for (const auto& [cluster, es] : esiac.Cluster2EndpointSet) {
                TProtoEndpointSet& protoEs = *protoEsiac.AddCluster2EndpointSet();
                protoEs.SetVersion(es.Version.GetOrElse(-1));
                protoEs.SetCluster(cluster);
                for (const ::grpc_resolved_address& address : es.Addresses) {
                    *protoEs.AddAddresses() = TStringBuf(address.addr, address.len);
                }
            }
        }

        return proto;
    }

    void LoadFromProto(const i64 version, ui8 const* const buffer, size_t bufferLength) {
        using TProtoEndpointSetInAllClusters = ::NStrm::NPackager::NResolveDataProto::TEndpointSetInAllClusters;
        using TProtoEndpointSet = ::NStrm::NPackager::NResolveDataProto::TEndpointSet;

        if (bufferLength == 0) {
            return;
        }

        TData& data = CacheData();
        const TGuard<TMutex> guard(data.Mutex);

        if (version <= data.Version) {
            return;
        }

        TDataProto proto;
        Y_ENSURE(proto.ParseFromArray(buffer, bufferLength));

        Y_ENSURE(version == proto.GetVersion());

        data.Version = proto.GetVersion();
        for (size_t i = 0; i < proto.Id2EndpointsSize(); ++i) {
            const TProtoEndpointSetInAllClusters& protoEsiac = proto.GetId2Endpoints(i);
            TEndpointSetInAllClusters& esiac = data.Id2Endpoints[protoEsiac.GetEndpointSetId()];

            esiac.Version = protoEsiac.GetVersion();

            for (size_t j = 0; j < protoEsiac.Cluster2EndpointSetSize(); ++j) {
                const TProtoEndpointSet& protoEs = protoEsiac.GetCluster2EndpointSet(j);
                TEndpointSet& es = esiac.Cluster2EndpointSet[protoEs.GetCluster()];
                es.Version = protoEs.GetVersion();

                esiac.Total += protoEs.AddressesSize() - es.Addresses.size();

                es.Addresses.resize(protoEs.AddressesSize());
                for (size_t k = 0; k < protoEs.AddressesSize(); ++k) {
                    const TString& address = protoEs.GetAddresses(k);
                    es.Addresses[k].len = address.length();
                    std::memcpy(es.Addresses[k].addr, address.c_str(), address.length());
                }
            }

            for (TResolver* sub : esiac.Subscribers) {
                sub->Update();
            }
        }
    }

}
