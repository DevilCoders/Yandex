#include "bootstrap.h"

#include "options.h"

#include <cloud/filestore/libs/diagnostics/config.h>
#include <cloud/filestore/libs/diagnostics/profile_log.h>
#include <cloud/filestore/libs/diagnostics/request_stats.h>
#include <cloud/filestore/libs/client/client.h>
#include <cloud/filestore/libs/client/config.h>
#include <cloud/filestore/libs/client/durable.h>
#include <cloud/filestore/libs/client/probes.h>
#include <cloud/filestore/libs/endpoint/listener.h>
#include <cloud/filestore/libs/endpoint/service.h>
#include <cloud/filestore/libs/endpoint_vhost/config.h>
#include <cloud/filestore/libs/endpoint_vhost/listener.h>
#include <cloud/filestore/libs/endpoint_vhost/server.h>
#include <cloud/filestore/libs/fuse/probes.h>
#include <cloud/filestore/libs/server/config.h>
#include <cloud/filestore/libs/server/probes.h>
#include <cloud/filestore/libs/server/server.h>
#include <cloud/filestore/libs/service/context.h>
#include <cloud/filestore/libs/service/endpoint.h>
#include <cloud/filestore/libs/service/filestore.h>
#include <cloud/filestore/libs/service/request.h>

#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/task_queue.h>
#include <cloud/storage/core/libs/common/thread_pool.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/daemon/mlock.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>
#include <cloud/storage/core/libs/diagnostics/stats_updater.h>
#include <cloud/storage/core/libs/diagnostics/trace_processor.h>
#include <cloud/storage/core/libs/diagnostics/trace_serializer.h>
#include <cloud/storage/core/libs/keyring/endpoints.h>
#include <cloud/storage/core/libs/version/version.h>

#include <library/cpp/lwtrace/mon/mon_lwtrace.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/generic/guid.h>
#include <util/generic/map.h>
#include <util/stream/file.h>
#include <util/system/fs.h>

namespace NCloud::NFileStore::NServer {

using namespace NCloud::NFileStore::NVhost;

namespace {

////////////////////////////////////////////////////////////////////////////////

const TString TraceLoggerId = "st_trace_logger";
const TString SlowRequestsFilterId = "st_slow_requests_filter";

////////////////////////////////////////////////////////////////////////////////

template <typename T>
void ParseProtoTextFromFile(const TString& fileName, T& dst)
{
    TFileInput in(fileName);
    ParseFromTextFormat(in, dst);
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

class TFileStoreEndpoints final
    : public IFileStoreEndpoints
{
private:
    using TEndpointsMap = TMap<TString, IFileStoreServicePtr>;

private:
    ITimerPtr Timer;
    ISchedulerPtr Scheduler;
    ILoggingServicePtr Logging;

    TEndpointsMap Endpoints;

public:
    TFileStoreEndpoints(
            ITimerPtr timer,
            ISchedulerPtr scheduler,
            ILoggingServicePtr logging)
        : Timer(std::move(timer))
        , Scheduler(std::move(scheduler))
        , Logging(std::move(logging))
    {}

    void Start() override
    {
        for (const auto& [name, endpoint]: Endpoints) {
            endpoint->Start();
        }
    }

    void Stop() override
    {
        for (const auto& [name, endpoint]: Endpoints) {
            endpoint->Stop();
        }
    }

    IFileStoreServicePtr GetEndpoint(const TString& name) override
    {
        const auto* p = Endpoints.FindPtr(name);
        return p ? *p : nullptr;
    }

    bool AddEndpoint(const TString& name, const NProto::TClientConfig& config)
    {
        if (Endpoints.contains(name)) {
            return false;
        }

        auto clientConfig = std::make_shared<NClient::TClientConfig>(config);
        auto client = NClient::CreateDurableClient(
            Logging,
            Timer,
            Scheduler,
            NClient::CreateRetryPolicy(clientConfig),
            NClient::CreateFileStoreClient(clientConfig, Logging));

        Endpoints.emplace(name, std::move(client));
        return true;
    }

    bool Empty() const
    {
        return Endpoints.empty();
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TVhostServerBootstrap::TVhostServerBootstrap(TOptionsPtr options)
    : TBootstrap{options, "NFS_VHOST", "server"} // TODO(fyodor) replace server -> vhost
    , Options{options}
{}

TVhostServerBootstrap::~TVhostServerBootstrap()
{}

NServer::IServerPtr TVhostServerBootstrap::CreateServer()
{
    InitConfig();

    NVhost::InitLog(Logging);
    switch (VhostServiceConfig->GetEndpointStorageType()) {
        case NCloud::NProto::ENDPOINT_STORAGE_DEFAULT:
        case NCloud::NProto::ENDPOINT_STORAGE_KEYRING:
            EndpointStorage = CreateKeyringEndpointStorage(
                VhostServiceConfig->GetRootKeyringName(),
                VhostServiceConfig->GetEndpointsKeyringName());
            break;
        case NCloud::NProto::ENDPOINT_STORAGE_FILE:
            EndpointStorage = CreateFileEndpointStorage(
                VhostServiceConfig->GetEndpointStorageDir());
            break;
        default:
            Y_FAIL(
                "unsupported endpoint storage type %d",
                VhostServiceConfig->GetEndpointStorageType());
    }

    switch (Options->Service) {
        case NDaemon::EServiceKind::Local:
            InitEndpoints();
            break;
        case NDaemon::EServiceKind::Null:
            InitNullEndpoints();
            break;
        case NDaemon::EServiceKind::Kikimr:
            Y_FAIL("Unsupported service kind: kikimr");
    }

    return NServer::CreateServer(
        ServerConfig,
        Logging,
        StatsRegistry->GetServerStats(),
        CreateProfileLogStub(),
        EndpointManager);
}

TVector<IIncompleteRequestProviderPtr> TVhostServerBootstrap::GetIncompleteRequestProviders()
{
    return {
        Server,
        EndpointManager,
    };
}

void TVhostServerBootstrap::InitConfig()
{
    NProto::TVhostAppConfig appConfig;
    if (Options->AppConfig) {
        ParseProtoTextFromFile(Options->AppConfig, appConfig);
    }

    NProto::TServerConfig& serverConfig = *appConfig.MutableServerConfig();
    if (Options->ServerAddress) {
        serverConfig.SetHost(Options->ServerAddress);
    }
    if (Options->ServerPort) {
        serverConfig.SetPort(Options->ServerPort);
    }

    VhostServiceConfig = std::make_shared<TVhostServiceConfig>(
        appConfig.GetVhostServiceConfig());
    ServerConfig = std::make_shared<TServerConfig>(serverConfig);
}

void TVhostServerBootstrap::InitEndpoints()
{
    auto endpoints = std::make_shared<TFileStoreEndpoints>(
        Timer,
        Scheduler,
        Logging);

    for (const auto& endpoint: VhostServiceConfig->GetServiceEndpoints()) {
        bool inserted = endpoints->AddEndpoint(endpoint.GetName(), endpoint.GetClientConfig());
        if (inserted) {
            STORAGE_INFO("configured endpoint %s -> %s:%u",
                endpoint.GetName().c_str(),
                endpoint.GetClientConfig().GetHost().c_str(),
                endpoint.GetClientConfig().GetPort());
        } else {
            STORAGE_ERROR("duplicated client config: " << endpoint.GetName());
        }
    }

    if (endpoints->Empty()) {
        NProto::TClientConfig clientConfig;
        if (Options->ConnectAddress) {
            clientConfig.SetHost(Options->ConnectAddress);
        }
        if (Options->ConnectPort) {
            clientConfig.SetPort(Options->ConnectPort);
        }

        bool inserted = endpoints->AddEndpoint({}, clientConfig);
        Y_VERIFY(inserted);

        STORAGE_WARN("configured default endpoint -> %s:%u",
            clientConfig.GetHost().c_str(),
            clientConfig.GetPort());
    }

    FileStoreEndpoints = std::move(endpoints);

    EndpointListener = NVhost::CreateEndpointListener(
        Logging,
        Timer,
        Scheduler,
        StatsRegistry,
        FileStoreEndpoints);

    EndpointManager = CreateEndpointManager(
        Logging,
        EndpointStorage,
        EndpointListener);
}

void TVhostServerBootstrap::InitNullEndpoints()
{
    EndpointManager = CreateNullEndpointManager();
}

void TVhostServerBootstrap::InitLWTrace()
{
    auto& probes = NLwTraceMonPage::ProbeRegistry();
    probes.AddProbesList(LWTRACE_GET_PROBES(FILESTORE_FUSE_PROVIDER));
    probes.AddProbesList(LWTRACE_GET_PROBES(FILESTORE_CLIENT_PROVIDER));
    probes.AddProbesList(LWTRACE_GET_PROBES(FILESTORE_SERVER_PROVIDER));

    auto& lwManager = NLwTraceMonPage::TraceManager(false);

    const TVector<std::tuple<TString, TString>> desc = {
        {"RequestReceived",                 "FILESTORE_FUSE_PROVIDER"},
    };

    auto traceLog = CreateUnifiedAgentLoggingService(
        Logging,
        DiagnosticsConfig->GetTracesUnifiedAgentEndpoint(),
        DiagnosticsConfig->GetTracesSyslogIdentifier()
    );

    if (auto samplingRate = DiagnosticsConfig->GetSamplingRate()) {
        NLWTrace::TQuery query = ProbabilisticQuery(
            desc,
            samplingRate,
            DiagnosticsConfig->GetLWTraceShuttleCount());
        lwManager.New(TraceLoggerId, query);
        TraceReaders.push_back(CreateTraceLogger(TraceLoggerId, traceLog, "NFS_TRACE"));
    }

    if (auto samplingRate = DiagnosticsConfig->GetSlowRequestSamplingRate()) {
        NLWTrace::TQuery query = ProbabilisticQuery(
            desc,
            samplingRate,
            DiagnosticsConfig->GetLWTraceShuttleCount());
        lwManager.New(SlowRequestsFilterId, query);
        TraceReaders.push_back(CreateSlowRequestsFilter(
            SlowRequestsFilterId,
            traceLog,
            "NFS_TRACE",
            DiagnosticsConfig->GetHDDSlowRequestThreshold(),
            DiagnosticsConfig->GetSSDSlowRequestThreshold(),
            TDuration::Zero()));
    }
}

void TVhostServerBootstrap::StartComponents()
{
    NVhost::StartServer();

    FILESTORE_LOG_START_COMPONENT(Logging);
    FILESTORE_LOG_START_COMPONENT(Monitoring);
    FILESTORE_LOG_START_COMPONENT(TraceProcessor);
    FILESTORE_LOG_START_COMPONENT(BackgroundThreadPool);

    FILESTORE_LOG_START_COMPONENT(FileStoreEndpoints);
    FILESTORE_LOG_START_COMPONENT(EndpointManager);
    FILESTORE_LOG_START_COMPONENT(RequestStatsUpdater);
    FILESTORE_LOG_START_COMPONENT(Server);

    FILESTORE_LOG_START_COMPONENT(Scheduler);
    FILESTORE_LOG_START_COMPONENT(BackgroundScheduler);

    RestoreKeyringEndpoints();
}

void TVhostServerBootstrap::StopComponents()
{
    FILESTORE_LOG_STOP_COMPONENT(BackgroundScheduler);
    FILESTORE_LOG_STOP_COMPONENT(Scheduler);

    FILESTORE_LOG_STOP_COMPONENT(Server);
    FILESTORE_LOG_STOP_COMPONENT(RequestStatsUpdater);
    FILESTORE_LOG_STOP_COMPONENT(EndpointManager);
    FILESTORE_LOG_STOP_COMPONENT(FileStoreEndpoints);

    FILESTORE_LOG_STOP_COMPONENT(BackgroundThreadPool);
    FILESTORE_LOG_STOP_COMPONENT(TraceProcessor);
    FILESTORE_LOG_STOP_COMPONENT(Monitoring);
    FILESTORE_LOG_STOP_COMPONENT(Logging);

    NVhost::StopServer();
}

void TVhostServerBootstrap::RestoreKeyringEndpoints()
{
    auto idsOrError = EndpointStorage->GetEndpointIds();

    if (HasError(idsOrError)) {
        auto logLevel = TLOG_INFO;

        if (VhostServiceConfig->GetRequireEndpointsKeyring()) {
            logLevel = TLOG_ERR;
            // TODO: report critical error
        }

        STORAGE_LOG(logLevel, "Failed to get endpoints from storage: "
            << FormatError(idsOrError.GetError()));
        return;
    }

    const auto& storedIds = idsOrError.GetResult();
    STORAGE_INFO("Found " << storedIds.size() << " endpoints in storage");

    auto clientId = CreateGuidAsString();

    for (auto keyringId: storedIds) {
        auto requestOrError = EndpointStorage->GetEndpoint(keyringId);
        if (HasError(requestOrError)) {
            // TODO: report critical error
            STORAGE_ERROR("Failed to restore endpoint. ID: " << keyringId
                << ", error: " << FormatError(requestOrError.GetError()));
            continue;
        }

        auto request = DeserializeEndpoint<NProto::TStartEndpointRequest>(
            requestOrError.GetResult());

        if (!request) {
            // TODO: report critical error
            STORAGE_ERROR("Failed to deserialize request. ID: " << keyringId);
            continue;
        }

        auto requestId = CreateRequestId();
        request->MutableHeaders()->SetRequestId(requestId);
        request->MutableHeaders()->SetClientId(clientId);

        auto future = EndpointManager->StartEndpoint(
            MakeIntrusive<TCallContext>(requestId),
            std::move(request));

        future.Subscribe([=] (const auto& f) {
            const auto& response = f.GetValue();
            if (HasError(response)) {
                // TODO: report critical error
                STORAGE_ERROR("Failed to start endpoint: "
                    << FormatError(response.GetError()));
            }
        });
    }
}

}   // namespace NCloud::NFileStore::NServer
