#include "bootstrap.h"

#include "config_initializer.h"
#include "options.h"

#include <cloud/blockstore/libs/client/client.h>
#include <cloud/blockstore/libs/client/config.h>
#include <cloud/blockstore/libs/common/caching_allocator.h>
#include <cloud/blockstore/libs/diagnostics/block_digest.h>
#include <cloud/blockstore/libs/diagnostics/config.h>
#include <cloud/blockstore/libs/diagnostics/cgroup_stats_fetcher.h>
#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/diagnostics/fault_injection.h>
#include <cloud/blockstore/libs/diagnostics/probes.h>
#include <cloud/blockstore/libs/diagnostics/profile_log.h>
#include <cloud/blockstore/libs/diagnostics/request_stats.h>
#include <cloud/blockstore/libs/diagnostics/server_stats.h>
#include <cloud/blockstore/libs/diagnostics/stats_aggregator.h>
#include <cloud/blockstore/libs/diagnostics/volume_stats.h>
#include <cloud/blockstore/libs/discovery/balancing.h>
#include <cloud/blockstore/libs/discovery/ban.h>
#include <cloud/blockstore/libs/discovery/config.h>
#include <cloud/blockstore/libs/discovery/discovery.h>
#include <cloud/blockstore/libs/discovery/fetch.h>
#include <cloud/blockstore/libs/discovery/healthcheck.h>
#include <cloud/blockstore/libs/discovery/ping.h>
#include <cloud/blockstore/libs/endpoints/endpoint_listener.h>
#include <cloud/blockstore/libs/endpoints/endpoint_manager.h>
#include <cloud/blockstore/libs/endpoints/service_endpoint.h>
#include <cloud/blockstore/libs/endpoints/session_manager.h>
#include <cloud/blockstore/libs/endpoints_grpc/socket_endpoint_listener.h>
#include <cloud/blockstore/libs/endpoints_nbd/nbd_server.h>
#include <cloud/blockstore/libs/endpoints_rdma/rdma_server.h>
#include <cloud/blockstore/libs/endpoints_spdk/spdk_server.h>
#include <cloud/blockstore/libs/endpoints_vhost/vhost_server.h>
#include <cloud/blockstore/libs/logbroker/config.h>
#include <cloud/blockstore/libs/logbroker/logbroker.h>
#include <cloud/blockstore/libs/nbd/server.h>
#include <cloud/blockstore/libs/notify/config.h>
#include <cloud/blockstore/libs/notify/notify.h>
#include <cloud/blockstore/libs/nvme/nvme.h>
#include <cloud/blockstore/libs/rdma/client.h>
#include <cloud/blockstore/libs/rdma/server.h>
#include <cloud/blockstore/libs/rdma/verbs.h>
#include <cloud/blockstore/libs/server/config.h>
#include <cloud/blockstore/libs/server/server.h>
#include <cloud/blockstore/libs/service/device_handler.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/service/service.h>
#include <cloud/blockstore/libs/service/service_auth.h>
#include <cloud/blockstore/libs/service/service_null.h>
#include <cloud/blockstore/libs/service/storage_provider.h>
#include <cloud/blockstore/libs/service_kikimr/auth_provider_kikimr.h>
#include <cloud/blockstore/libs/service_kikimr/service_kikimr.h>
#include <cloud/blockstore/libs/service_local/service_local.h>
#include <cloud/blockstore/libs/service_local/storage_aio.h>
#include <cloud/blockstore/libs/service_local/storage_null.h>
#include <cloud/blockstore/libs/service_local/storage_rdma.h>
#include <cloud/blockstore/libs/service_local/storage_spdk.h>
#include <cloud/blockstore/libs/service_throttling/throttler_metrics.h>
#include <cloud/blockstore/libs/service_throttling/throttler_policy.h>
#include <cloud/blockstore/libs/service_throttling/throttler_tracker.h>
#include <cloud/blockstore/libs/service_throttling/throttling.h>
#include <cloud/blockstore/libs/spdk/config.h>
#include <cloud/blockstore/libs/spdk/env.h>
#include <cloud/blockstore/libs/spdk/rdma.h>
#include <cloud/blockstore/libs/storage_local/config.h>
#include <cloud/blockstore/libs/storage_local/storage_local.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/features_config.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/libs/storage/disk_agent/config.h>
#include <cloud/blockstore/libs/storage/disk_agent/probes.h>
#include <cloud/blockstore/libs/storage/disk_registry_proxy/config.h>
#include <cloud/blockstore/libs/storage/init/actorsystem.h>
#include <cloud/blockstore/libs/storage/init/node.h>
#include <cloud/blockstore/libs/throttling/throttler.h>
#include <cloud/blockstore/libs/throttling/throttler_logger.h>
#include <cloud/blockstore/libs/validation/validation.h>
#include <cloud/blockstore/libs/vhost/server.h>
#include <cloud/blockstore/libs/vhost/vhost.h>
#include <cloud/blockstore/libs/ydbstats/config.h>
#include <cloud/blockstore/libs/ydbstats/ydbscheme.h>
#include <cloud/blockstore/libs/ydbstats/ydbstats.h>
#include <cloud/blockstore/libs/ydbstats/ydbstorage.h>

#include <cloud/storage/core/libs/aio/service.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/file_io_service.h>
#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/thread_pool.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/coroutine/executor.h>
#include <cloud/storage/core/libs/daemon/mlock.h>
#include <cloud/storage/core/libs/diagnostics/critical_events.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>
#include <cloud/storage/core/libs/diagnostics/stats_updater.h>
#include <cloud/storage/core/libs/diagnostics/trace_processor.h>
#include <cloud/storage/core/libs/diagnostics/trace_serializer.h>
#include <cloud/storage/core/libs/grpc/executor.h>
#include <cloud/storage/core/libs/grpc/logging.h>
#include <cloud/storage/core/libs/grpc/threadpool.h>
#include <cloud/storage/core/libs/keyring/endpoints.h>
#include <cloud/storage/core/libs/kikimr/actorsystem.h>
#include <cloud/storage/core/libs/version/version.h>

#include <ydb/core/blobstorage/lwtrace_probes/blobstorage_probes.h>
#include <ydb/core/protos/config.pb.h>
#include <ydb/core/tablet_flat/probes.h>

#include <library/cpp/lwtrace/mon/mon_lwtrace.h>
#include <library/cpp/lwtrace/probes.h>
#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/datetime/base.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/system/hostname.h>

namespace NCloud::NBlockStore::NServer {

using namespace NActors;
using namespace NKikimr;
using namespace NMonitoring;
using namespace NNvme;

using namespace NCloud::NBlockStore::NDiscovery;
using namespace NCloud::NBlockStore::NStorage;

namespace {

////////////////////////////////////////////////////////////////////////////////

const TString TraceLoggerId = "st_trace_logger";
const TString SlowRequestsFilterId = "st_slow_requests_filter";

////////////////////////////////////////////////////////////////////////////////

NVhost::TServerConfig CreateVhostServerConfig(const TServerAppConfig& config)
{
    return NVhost::TServerConfig {
        .ThreadsCount = config.GetVhostThreadsCount(),
        .Affinity = config.GetVhostAffinity()
    };
}

NBD::TServerConfig CreateNbdServerConfig(const TServerAppConfig& config)
{
    return NBD::TServerConfig {
        .ThreadsCount = config.GetNbdThreadsCount(),
        .LimiterEnabled = config.GetNbdLimiterEnabled(),
        .MaxInFlightBytesPerThread = config.GetMaxInFlightBytesPerThread(),
        .Affinity = config.GetNbdAffinity()
    };
}

TNVMeEndpointConfig CreateNVMeEndpointConfig(const TServerAppConfig& config)
{
    return TNVMeEndpointConfig {
        .Nqn = config.GetNVMeEndpointNqn(),
        .TransportIDs = config.GetNVMeEndpointTransportIDs(),
    };
}

TSCSIEndpointConfig CreateSCSIEndpointConfig(const TServerAppConfig& config)
{
    return TSCSIEndpointConfig {
        .ListenAddress = config.GetSCSIEndpointListenAddress(),
        .ListenPort = config.GetSCSIEndpointListenPort(),
    };
}

TRdmaEndpointConfig CreateRdmaEndpointConfig(const TServerAppConfig& config)
{
    return TRdmaEndpointConfig {
        .ListenAddress = config.GetRdmaEndpointListenAddress(),
        .ListenPort = config.GetRdmaEndpointListenPort(),
    };
}

TThrottlingServiceConfig CreateThrottlingServicePolicyConfig(
    const TServerAppConfig& config)
{
    return TThrottlingServiceConfig(
        config.GetMaxReadBandwidth(),
        config.GetMaxWriteBandwidth(),
        config.GetMaxReadIops(),
        config.GetMaxWriteIops(),
        config.GetMaxBurstTime()
    );
}

NRdma::EWaitMode ConvertWaitMode(NProto::EWaitMode mode)
{
    switch (mode) {
        case NProto::WAIT_MODE_POLL:
            return NRdma::EWaitMode::Poll;

        case NProto::WAIT_MODE_BUSY_WAIT:
            return NRdma::EWaitMode::BusyWait;

        case NProto::WAIT_MODE_ADAPTIVE_WAIT:
            return NRdma::EWaitMode::AdaptiveWait;;

        default:
            Y_FAIL("unsupported wait mode %d", mode);
    }
}

NRdma::TClientConfigPtr CreateRdmaClientConfig(const TServerAppConfigPtr app)
{
    auto server = app->GetServerConfig();
    auto config = std::make_shared<NRdma::TClientConfig>();

    if (server->HasRdmaClientConfig()) {
        auto client = server->GetRdmaClientConfig();

        config->QueueSize = client.GetQueueSize();
        config->MaxBufferSize = client.GetMaxBufferSize();
        config->WaitMode = ConvertWaitMode(client.GetWaitMode());
        config->PollerThreads = client.GetPollerThreads();
    };

    return config;
}

////////////////////////////////////////////////////////////////////////////////

class TLoggingProxy final
    : public ILoggingService
{
private:
    IActorSystemPtr ActorSystem;

public:
    void Init(IActorSystemPtr actorSystem)
    {
        ActorSystem = std::move(actorSystem);
    }

    void Start() override
    {
        Y_VERIFY(ActorSystem);
    }

    void Stop() override
    {
        ActorSystem.Reset();
    }

    TLog CreateLog(const TString& component) override
    {
        return ActorSystem->CreateLog(component);
    }
};

////////////////////////////////////////////////////////////////////////////////

class TMonitoringProxy final
    : public IMonitoringService
{
private:
    IActorSystemPtr ActorSystem;

public:
    void Init(IActorSystemPtr actorSystem)
    {
        ActorSystem = std::move(actorSystem);
    }

    void Start() override
    {
        Y_VERIFY(ActorSystem);
    }

    void Stop() override
    {
        ActorSystem.Reset();
    }

    IMonPagePtr RegisterIndexPage(
        const TString& path,
        const TString& title) override
    {
        return ActorSystem->RegisterIndexPage(path, title);
    }

    void RegisterMonPage(IMonPagePtr page) override
    {
        ActorSystem->RegisterMonPage(page);
    }

    IMonPagePtr GetMonPage(const TString& path) override
    {
        return ActorSystem->GetMonPage(path);
    }

    TDynamicCountersPtr GetCounters() override
    {
        return ActorSystem->GetCounters();
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TBootstrap::TBootstrap(TOptionsPtr options)
    : Configs(new TConfigInitializer(std::move(options)))
{}

TBootstrap::~TBootstrap()
{}

void TBootstrap::Init(IDeviceHandlerFactoryPtr deviceHandlerFactory)
{
    TLogSettings logSettings;
    logSettings.UseLocalTimestamps = true;

    BootstrapLogging = CreateLoggingService("console", logSettings);
    Log = BootstrapLogging->CreateLog("BLOCKSTORE_SERVER");
    Configs->Log = Log;
    STORAGE_INFO("NBS server version: " << GetFullVersionString());

    Timer = CreateWallClockTimer();
    Scheduler = CreateScheduler();
    BackgroundThreadPool = CreateThreadPool("Background", 1);
    BackgroundScheduler = CreateBackgroundScheduler(
        Scheduler,
        BackgroundThreadPool);

    switch (Configs->Options->ServiceKind) {
        case TOptions::EServiceKind::Kikimr:
            InitKikimrService();
            break;
        case TOptions::EServiceKind::Local:
            InitLocalService();
            break;
        case TOptions::EServiceKind::Null:
            InitNullService();
            break;
    }

    STORAGE_INFO("Service initialized");

    GrpcLog = Logging->CreateLog("GRPC");
    GrpcLoggerInit(
        GrpcLog,
        Configs->Options->EnableGrpcTracing);

    auto diagnosticsConfig = Configs->DiagnosticsConfig;
    if (TraceReaders.size()) {
        TraceProcessor = CreateTraceProcessor(
            Timer,
            BackgroundScheduler,
            Logging,
            Monitoring,
            "BLOCKSTORE_TRACE",
            NLwTraceMonPage::TraceManager(diagnosticsConfig->GetUnsafeLWTrace()),
            TraceReaders);

        STORAGE_INFO("TraceProcessor initialized");
    }

    auto clientInactivityTimeout = Configs->StorageConfig->GetInactiveClientsTimeout();

    auto rootGroup = Monitoring->GetCounters()
        ->GetSubgroup("counters", "blockstore");

    auto serverGroup = rootGroup->GetSubgroup("component", "server");
    auto revisionGroup = serverGroup->GetSubgroup("revision", GetFullVersionString());

    auto versionCounter = revisionGroup->GetCounter(
        "version",
        false);
    *versionCounter = 1;

    InitCriticalEventsCounter(serverGroup);

    for (auto& event: PostponedCriticalEvents) {
        ReportCriticalEvent(
            event,
            "",     // message
            false); // verifyDebug
    }
    PostponedCriticalEvents.clear();

    RequestStats = CreateServerRequestStats(serverGroup, Timer);

    if (!VolumeStats) {
        VolumeStats = CreateVolumeStats(
            Monitoring,
            Configs->DiagnosticsConfig,
            clientInactivityTimeout,
            EVolumeStatsType::EServerStats,
            Timer);
    }

    ServerStats = CreateServerStats(
        Configs->ServerConfig,
        Configs->DiagnosticsConfig,
        Monitoring,
        ProfileLog,
        RequestStats,
        VolumeStats);

    STORAGE_INFO("Stats initialized");

    TVector<IStorageProviderPtr> storageProviders;

    if (Configs->LocalStorageConfig->GetEnabled()) {
        STORAGE_INFO("Initialize Local Storage Provider");

        storageProviders.push_back(NServer::CreateLocalStorageProvider(
            FQDNHostName(),
            AioStorageProvider,
            Logging,
            ServerStats));
    }

    if (Configs->ServerConfig->GetNvmfInitiatorEnabled()) {
        Y_VERIFY(Spdk);

        const auto& config = *Configs->DiskAgentConfig;

        storageProviders.push_back(CreateSpdkStorageProvider(
            Spdk,
            CreateSyncCachingAllocator(
                Spdk->GetAllocator(),
                config.GetPageSize(),
                config.GetMaxPageCount(),
                config.GetPageDropSize()),
            ServerStats));
    }

    if (!Configs->StorageConfig->GetUseNonreplicatedRdmaActor() && RdmaClient) {
        storageProviders.push_back(CreateRdmaStorageProvider(
            ServerStats,
            RdmaClient));
    }

    storageProviders.push_back(CreateDefaultStorageProvider(Service));
    StorageProvider = CreateMultiStorageProvider(std::move(storageProviders));

    STORAGE_INFO("StorageProvider initialized");

    TSessionManagerOptions sessionManagerOptions;
    sessionManagerOptions.StrictContractValidation
        = Configs->ServerConfig->GetStrictContractValidation();
    sessionManagerOptions.DefaultClientConfig
        = Configs->EndpointConfig->GetClientConfig();
    sessionManagerOptions.HostProfile = Configs->HostPerformanceProfile;

    Executor = TExecutor::Create("SVC");

    auto sessionManager = CreateSessionManager(
        Timer,
        Scheduler,
        Logging,
        Monitoring,
        RequestStats,
        VolumeStats,
        ServerStats,
        Service,
        StorageProvider,
        Executor,
        sessionManagerOptions);

    STORAGE_INFO("SessionManager initialized");

    THashMap<NProto::EClientIpcType, IEndpointListenerPtr> endpointListeners;

    GrpcEndpointListener = CreateSocketEndpointListener(
        Logging,
        Configs->ServerConfig->GetUnixSocketBacklog());
    endpointListeners.emplace(NProto::IPC_GRPC, GrpcEndpointListener);

    STORAGE_INFO("SocketEndpointListener initialized");

    if (Configs->ServerConfig->GetNbdEnabled()) {
        NbdServer = NBD::CreateServer(
            Logging,
            CreateNbdServerConfig(*Configs->ServerConfig));

        STORAGE_INFO("NBD Server initialized");

        auto nbdEndpointListener = CreateNbdEndpointListener(
            NbdServer,
            Logging,
            ServerStats);

        endpointListeners.emplace(
            NProto::IPC_NBD,
            std::move(nbdEndpointListener));

        STORAGE_INFO("NBD EndpointListener initialized");
    }

    if (Configs->ServerConfig->GetVhostEnabled()) {
        NVhost::InitVhostLog(Logging);

        if (!deviceHandlerFactory) {
            deviceHandlerFactory = CreateDefaultDeviceHandlerFactory();
        }

        VhostServer = NVhost::CreateServer(
            Logging,
            ServerStats,
            NVhost::CreateVhostQueueFactory(),
            deviceHandlerFactory,
            CreateVhostServerConfig(*Configs->ServerConfig),
            Rdma);

        STORAGE_INFO("VHOST Server initialized");

        auto vhostEndpointListener = CreateVhostEndpointListener(
            VhostServer);

        endpointListeners.emplace(
            NProto::IPC_VHOST,
            std::move(vhostEndpointListener));

        STORAGE_INFO("VHOST EndpointListener initialized");
    }

    if (Configs->ServerConfig->GetNVMeEndpointEnabled()) {
        Y_VERIFY(Spdk);

        auto listener = CreateNVMeEndpointListener(
            Spdk,
            Logging,
            ServerStats,
            Executor,
            CreateNVMeEndpointConfig(*Configs->ServerConfig));

        endpointListeners.emplace(
            NProto::IPC_NVME,
            std::move(listener));

        STORAGE_INFO("NVMe EndpointListener initialized");
    }

    if (Configs->ServerConfig->GetSCSIEndpointEnabled()) {
        Y_VERIFY(Spdk);

        auto listener = CreateSCSIEndpointListener(
            Spdk,
            Logging,
            ServerStats,
            Executor,
            CreateSCSIEndpointConfig(*Configs->ServerConfig));

        endpointListeners.emplace(
            NProto::IPC_SCSI,
            std::move(listener));

        STORAGE_INFO("SCSI EndpointListener initialized");
    }

    if (Configs->ServerConfig->GetRdmaEndpointEnabled()) {
        auto rdmaConfig = std::make_shared<NRdma::TServerConfig>();
        // TODO

        RdmaServer = NRdma::CreateServer(
            Logging,
            Monitoring,
            std::move(rdmaConfig));

        STORAGE_INFO("RDMA Server initialized");

        RdmaThreadPool = CreateThreadPool("RDMA", 1);
        auto listener = CreateRdmaEndpointListener(
            RdmaServer,
            Logging,
            ServerStats,
            Executor,
            RdmaThreadPool,
            CreateRdmaEndpointConfig(*Configs->ServerConfig));

        endpointListeners.emplace(
            NProto::IPC_RDMA,
            std::move(listener));

        STORAGE_INFO("RDMA EndpointListener initialized");
    }

    auto endpointManager = CreateEndpointManager(
        Logging,
        ServerStats,
        Executor,
        std::move(sessionManager),
        std::move(endpointListeners),
        Configs->ServerConfig->GetNbdSocketSuffix());

    STORAGE_INFO("EndpointManager initialized");

    IEndpointStoragePtr endpointStorage;
    switch (Configs->ServerConfig->GetEndpointStorageType()) {
        case NCloud::NProto::ENDPOINT_STORAGE_DEFAULT:
        case NCloud::NProto::ENDPOINT_STORAGE_KEYRING:
            endpointStorage = CreateKeyringEndpointStorage(
                Configs->ServerConfig->GetRootKeyringName(),
                Configs->ServerConfig->GetEndpointsKeyringName());
            break;
        case NCloud::NProto::ENDPOINT_STORAGE_FILE:
            endpointStorage = CreateFileEndpointStorage(
                Configs->ServerConfig->GetEndpointStorageDir());
            break;
        default:
            Y_FAIL(
                "unsupported endpoint storage type %d",
                Configs->ServerConfig->GetEndpointStorageType());
    }
    STORAGE_INFO("EndpointStorage initialized");

    EndpointService = CreateMultipleEndpointService(
        std::move(Service),
        Timer,
        Scheduler,
        Logging,
        RequestStats,
        VolumeStats,
        ServerStats,
        std::move(endpointStorage),
        std::move(endpointManager),
        Configs->EndpointConfig->GetClientConfig());
    Service = EndpointService;

    STORAGE_INFO("MultipleEndpointService initialized");

    if (Configs->ServerConfig->GetThrottlingEnabled()) {
        Service = CreateThrottlingService(
            std::move(Service),
            CreateThrottler(
                CreateThrottlerLoggerDefault(
                    RequestStats,
                    Logging,
                    "BLOCKSTORE_SERVER"),
                CreateThrottlingServiceMetrics(
                    Timer,
                    rootGroup,
                    Logging),
                CreateThrottlingServicePolicy(
                    CreateThrottlingServicePolicyConfig(
                        *Configs->ServerConfig)),
                CreateThrottlingServiceTracker(),
                Timer,
                Scheduler,
                VolumeStats));

        STORAGE_INFO("ThrottlingService initialized");
    }

    if (ActorSystem) {
        Service = CreateAuthService(
            std::move(Service),
            CreateKikimrAuthProvider(ActorSystem));

        STORAGE_INFO("AuthService initialized");
    }

    if (Configs->ServerConfig->GetStrictContractValidation()) {
        Service = CreateValidationService(
            Logging,
            Monitoring,
            std::move(Service),
            CreateCrcDigestCalculator(),
            clientInactivityTimeout);

        STORAGE_INFO("ValidationService initialized");
    }

    Server = CreateServer(
        Configs->ServerConfig,
        Logging,
        ServerStats,
        Service);

    STORAGE_INFO("Server initialized");

    GrpcEndpointListener->SetClientAcceptor(Server->GetClientAcceptor());

    TVector<IIncompleteRequestProviderPtr> requestProviders = {
        Server,
        EndpointService
    };

    if (NbdServer) {
        requestProviders.push_back(NbdServer);
    }

    if (VhostServer) {
        requestProviders.push_back(VhostServer);
    }

    ServerStatsUpdater = CreateStatsUpdater(
        Timer,
        BackgroundScheduler,
        ServerStats,
        std::move(requestProviders));

    STORAGE_INFO("ServerStatsUpdater initialized");
}

void TBootstrap::InitSpdk()
{
    const bool needSpdkForInitiator =
        Configs->ServerConfig->GetNvmfInitiatorEnabled();

    const bool needSpdkForTarget =
        Configs->ServerConfig->GetNVMeEndpointEnabled() ||
        Configs->ServerConfig->GetSCSIEndpointEnabled();

    const bool needSpdkForDiskAgent =
        Configs->DiskAgentConfig->GetEnabled() &&
        Configs->DiskAgentConfig->GetBackend() == NProto::DISK_AGENT_BACKEND_SPDK;

    if (needSpdkForInitiator || needSpdkForTarget || needSpdkForDiskAgent) {
        Spdk = NSpdk::CreateEnv(Configs->SpdkEnvConfig);

        STORAGE_INFO("Spdk initialized");
    }

    Rdma = Spdk ? NSpdk::RdmaHandler() : NRdma::RdmaHandlerStub();
}

void TBootstrap::InitProfileLog()
{
    if (Configs->Options->ProfileFile) {
        ProfileLog = CreateProfileLog(
            {
                Configs->Options->ProfileFile,
                Configs->DiagnosticsConfig->GetProfileLogTimeThreshold(),
            },
            Timer,
            BackgroundScheduler
        );
    } else {
        ProfileLog = CreateProfileLogStub();
    }
}

void TBootstrap::InitRdmaClient()
{
    try {
        if (Configs->ServerConfig->GetRdmaClientEnabled()) {
            RdmaClient = NRdma::CreateClient(
                Logging,
                Monitoring,
                CreateRdmaClientConfig(Configs->ServerConfig));

            STORAGE_INFO("RDMA client initialized");
        }
    } catch (...) {
        STORAGE_ERROR("Failed to initialize RDMA client: "
            << CurrentExceptionMessage().c_str());

        RdmaClient = nullptr;
        PostponedCriticalEvents.push_back("AppCriticalEvents/RdmaError");
    }
}

void TBootstrap::InitConfigs()
{
    Configs->InitKikimrConfig();
    Configs->InitServerConfig();
    Configs->InitEndpointConfig();
    Configs->InitHostPerformanceProfile();
    Configs->InitFeaturesConfig();
    Configs->InitStorageConfig();
    Configs->InitDiskRegistryProxyConfig();
    Configs->InitDiagnosticsConfig();
    Configs->InitStatsUploadConfig();
    Configs->InitDiscoveryConfig();
    Configs->InitSpdkEnvConfig();
    Configs->InitLogbrokerConfig();
    Configs->InitNotifyConfig();
}

void TBootstrap::InitKikimrService()
{
    InitConfigs();

    NStorage::TRegisterDynamicNodeOptions registerOpts;
    registerOpts.Domain = Configs->Options->Domain;
    registerOpts.SchemeShardDir = Configs->StorageConfig->GetSchemeShardDir();
    registerOpts.NodeType = Configs->ServerConfig->GetNodeType();
    registerOpts.NodeBrokerAddress = Configs->Options->NodeBrokerAddress;
    registerOpts.NodeBrokerPort = Configs->Options->NodeBrokerPort;
    registerOpts.InterconnectPort = Configs->Options->InterconnectPort;
    registerOpts.LoadCmsConfigs = Configs->ServerConfig->GetLoadCmsConfigs();
    registerOpts.MaxAttempts =
        Configs->ServerConfig->GetNodeRegistrationMaxAttempts();
    registerOpts.RegistrationTimeout =
        Configs->ServerConfig->GetNodeRegistrationTimeout();
    registerOpts.ErrorTimeout =
        Configs->ServerConfig->GetNodeRegistrationErrorTimeout();

    if (Configs->Options->LocationFile) {
        NProto::TLocation location;
        ParseProtoTextFromFile(Configs->Options->LocationFile, location);

        registerOpts.DataCenter = location.GetDataCenter();
        Configs->Rack = location.GetRack();
    }

    Configs->InitLocalStorageConfig();
    Configs->InitDiskAgentConfig();

    STORAGE_INFO("Configs initialized");

    auto [nodeId, scopeId, cmsConfig] = NStorage::RegisterDynamicNode(
        Configs->KikimrConfig,
        registerOpts,
        Log);

    if (cmsConfig) {
        Configs->ApplyCMSConfigs(std::move(*cmsConfig));
    }

    STORAGE_INFO("CMS configs initialized");

    auto logging = std::make_shared<TLoggingProxy>();
    auto monitoring = std::make_shared<TMonitoringProxy>();

    Logging = logging;
    Monitoring = monitoring;

    InitRdmaClient();

    VolumeStats = CreateVolumeStats(
        monitoring,
        Configs->DiagnosticsConfig,
        Configs->StorageConfig->GetInactiveClientsTimeout(),
        EVolumeStatsType::EServerStats,
        Timer);

    ClientPercentiles = CreateClientPercentileCalculator(logging);

    STORAGE_INFO("ClientPercentiles initialized");

    StatsAggregator = CreateStatsAggregator(
        Timer,
        BackgroundScheduler,
        logging,
        monitoring,
        [=] (
            NMonitoring::TDynamicCountersPtr updatedCounters,
            NMonitoring::TDynamicCountersPtr baseCounters)
        {
            ClientPercentiles->CalculatePercentiles(updatedCounters);
            UpdateClientStats(updatedCounters, baseCounters);
        });

    STORAGE_INFO("StatsAggregator initialized");

    auto statsConfig = Configs->StatsConfig;
    if (statsConfig->IsValid()) {
        YdbStorage = NYdbStats::CreateYdbStorage(statsConfig, logging);
        StatsUploader = NYdbStats::CreateYdbVolumesStatsUploader(
            statsConfig,
            logging,
            YdbStorage,
            NYdbStats::CreateStatsTableScheme(),
            NYdbStats::CreateHistoryTableScheme(),
            NYdbStats::CreateArchiveStatsTableScheme(),
            NYdbStats::CreateBlobLoadMetricsTableScheme());
    } else {
        StatsUploader = NYdbStats::CreateVolumesStatsUploaderStub();
    }

    STORAGE_INFO("StatsUploader initialized");

    auto discoveryConfig = Configs->DiscoveryConfig;
    if (discoveryConfig->GetConductorGroups()
            || discoveryConfig->GetInstanceListFile())
    {
        auto banList = discoveryConfig->GetBannedInstanceListFile()
            ? CreateBanList(discoveryConfig, logging, monitoring)
            : CreateBanListStub();
        auto staticFetcher = discoveryConfig->GetInstanceListFile()
            ? CreateStaticInstanceFetcher(
                discoveryConfig,
                logging,
                monitoring
            )
            : CreateInstanceFetcherStub();
        auto conductorFetcher = discoveryConfig->GetConductorGroups()
            ? CreateConductorInstanceFetcher(
                discoveryConfig,
                logging,
                monitoring,
                Timer,
                Scheduler
            )
            : CreateInstanceFetcherStub();

        auto healthChecker = CreateHealthChecker(
                discoveryConfig,
                logging,
                monitoring,
                CreateInsecurePingClient(),
                CreateSecurePingClient(Configs->ServerConfig->GetRootCertsFile())
        );

        auto balancingPolicy = CreateBalancingPolicy();

        DiscoveryService = CreateDiscoveryService(
            discoveryConfig,
            Timer,
            Scheduler,
            logging,
            monitoring,
            std::move(banList),
            std::move(staticFetcher),
            std::move(conductorFetcher),
            std::move(healthChecker),
            std::move(balancingPolicy)
        );
    } else {
        DiscoveryService = CreateDiscoveryServiceStub(
            FQDNHostName(),
            discoveryConfig->GetConductorInstancePort(),
            discoveryConfig->GetConductorSecureInstancePort()
        );
    }

    STORAGE_INFO("DiscoveryService initialized");

    TraceSerializer = CreateTraceSerializer(
        logging,
        "BLOCKSTORE_TRACE",
        NLwTraceMonPage::TraceManager(false));

    STORAGE_INFO("TraceSerializer initialized");

    if (Configs->DiagnosticsConfig->GetUseAsyncLogger()) {
        AsyncLogger = CreateAsyncLogger();

        STORAGE_INFO("AsyncLogger initialized");
    }

    InitSpdk();

    FileIOService = CreateAIOService();

    if (auto& config = *Configs->LocalStorageConfig; config.GetEnabled()) {
        const TString submissionQueueName = config.GetSingleQueue()
            ? "IO/Q"
            : "IO/SQ";

        SubmissionQueue = config.GetSubmissionQueueThreadCount() > 0
            ? CreateThreadPool(
                submissionQueueName,
                config.GetSubmissionQueueThreadCount())
            : CreateTaskQueueStub();

        ITaskQueuePtr completionQueue = SubmissionQueue;

        if (!config.GetSingleQueue()) {
            CompletionQueue = config.GetCompletionQueueThreadCount() > 0
                ? CreateThreadPool("IO/CQ", config.GetCompletionQueueThreadCount())
                : CreateTaskQueueStub();
            completionQueue = CompletionQueue;
        }

         IFileIOServicePtr fileIO;

        if (config.GetBackend() == NProto::LOCAL_STORAGE_BACKEND_AIO) {
            fileIO = FileIOService;
            NvmeManager = CreateNvmeManager(
                Configs->DiskAgentConfig->GetSecureEraseTimeout());
        } else {
            fileIO = CreateFileIOServiceStub();
            NvmeManager = CreateNvmeManagerStub();
        }

        AioStorageProvider = CreateAioStorageProvider(
            std::move(fileIO),
            SubmissionQueue,
            completionQueue,
            NvmeManager,
            !config.GetDirectIoDisabled());

        STORAGE_INFO("AioStorageProvider initialized");
    }

    if (Configs->DiskAgentConfig->GetEnabled() &&
        Configs->DiskAgentConfig->GetBackend() == NProto::DISK_AGENT_BACKEND_AIO &&
        !AioStorageProvider)
    {
        Y_VERIFY(FileIOService);

        NvmeManager = CreateNvmeManager(
            Configs->DiskAgentConfig->GetSecureEraseTimeout());

        AioStorageProvider = CreateAioStorageProvider(
            FileIOService,
            NvmeManager,
            !Configs->DiskAgentConfig->GetDirectIoFlagDisabled());

        STORAGE_INFO("AioStorageProvider initialized");
    }

    if (Configs->DiskAgentConfig->GetEnabled() &&
        Configs->DiskAgentConfig->GetBackend() == NProto::DISK_AGENT_BACKEND_NULL &&
        !AioStorageProvider)
    {
        AioStorageProvider = CreateNullStorageProvider();

        STORAGE_INFO("AioStorageProvider (null) initialized");
    }

    Y_VERIFY(FileIOService);

    Allocator = CreateCachingAllocator(
        Spdk ? Spdk->GetAllocator() : TDefaultAllocator::Instance(),
        Configs->DiskAgentConfig->GetPageSize(),
        Configs->DiskAgentConfig->GetMaxPageCount(),
        Configs->DiskAgentConfig->GetPageDropSize());

    STORAGE_INFO("Allocator initialized");

    InitProfileLog();

    STORAGE_INFO("ProfileLog initialized");

    CgroupStatsFetcher = CreateCgroupStatsFetcher(
        logging,
        monitoring,
        Configs->DiagnosticsConfig->GetCpuWaitFilename());

    if (Configs->StorageConfig->GetBlockDigestsEnabled()) {
        if (Configs->StorageConfig->GetUseTestBlockDigestGenerator()) {
            BlockDigestGenerator = CreateTestBlockDigestGenerator();
        } else {
            BlockDigestGenerator = CreateExt4BlockDigestGenerator(
                Configs->StorageConfig->GetDigestedBlocksPercentage());
        }
    } else {
        BlockDigestGenerator = CreateBlockDigestGeneratorStub();
    }

    STORAGE_INFO("DigestGenerator initialized");

    LogbrokerService = Configs->LogbrokerConfig->GetAddress()
        ? NLogbroker::CreateService(Configs->LogbrokerConfig, logging)
        : NLogbroker::CreateServiceNull(logging);

    STORAGE_INFO("LogbrokerService initialized");

    NotifyService = Configs->NotifyConfig->GetEndpoint()
        ? NNotify::CreateService(Configs->NotifyConfig)
        : NNotify::CreateNullService(logging);

    STORAGE_INFO("NotifyService initialized");

    NStorage::TActorSystemArgs args;
    args.NodeId = nodeId;
    args.ScopeId = scopeId;
    args.AppConfig = Configs->KikimrConfig;
    args.DiagnosticsConfig = Configs->DiagnosticsConfig;
    args.StorageConfig = Configs->StorageConfig;
    args.DiskAgentConfig = Configs->DiskAgentConfig;
    args.DiskRegistryProxyConfig = Configs->DiskRegistryProxyConfig;
    args.AsyncLogger = AsyncLogger;
    args.StatsAggregator = StatsAggregator;
    args.StatsUploader = StatsUploader;
    args.DiscoveryService = DiscoveryService;
    args.Spdk = Spdk;
    args.Allocator = Allocator;
    args.FileIOService = FileIOService;
    args.AioStorageProvider = AioStorageProvider;
    args.ProfileLog = ProfileLog;
    args.BlockDigestGenerator = BlockDigestGenerator;
    args.TraceSerializer = TraceSerializer;
    args.LogbrokerService = LogbrokerService;
    args.NotifyService = NotifyService;
    args.VolumeStats = VolumeStats;
    args.CgroupStatsFetcher = CgroupStatsFetcher;
    args.RdmaServer = nullptr;
    args.RdmaClient = RdmaClient;
    args.Logging = logging;

    ActorSystem = NStorage::CreateActorSystem(args);

    STORAGE_INFO("ActorSystem initialized");

    logging->Init(ActorSystem);
    monitoring->Init(ActorSystem);

    InitLWTrace();

    STORAGE_INFO("LWTrace initialized");

    SpdkLog = Logging->CreateLog("BLOCKSTORE_SPDK");
    NSpdk::InitLogging(SpdkLog);

    const auto& config = Configs->ServerConfig->GetKikimrServiceConfig()
        ? *Configs->ServerConfig->GetKikimrServiceConfig()
        : NProto::TKikimrServiceConfig();

    Service = CreateKikimrService(ActorSystem, config);
}

void TBootstrap::InitDbgConfigs()
{
    Configs->InitServerConfig();
    Configs->InitEndpointConfig();
    Configs->InitHostPerformanceProfile();
    Configs->InitStorageConfig();
    Configs->InitLocalStorageConfig();
    Configs->InitDiskAgentConfig();
    Configs->InitDiskRegistryProxyConfig();
    Configs->InitDiagnosticsConfig();
    Configs->InitDiscoveryConfig();
    Configs->InitSpdkEnvConfig();

    const auto monConfig = Configs->GetMonitoringConfig();
    const auto logConfig = Configs->GetLogConfig();

    TLogSettings logSettings;
    logSettings.UseLocalTimestamps = true;
    logSettings.FiltrationLevel =
        static_cast<ELogPriority>(logConfig.GetDefaultLevel());

    Logging = CreateLoggingService("console", logSettings);

    InitLWTrace();

    ui32 monPort = monConfig.GetMonitoringPort();
    if (monPort) {
        TString monAddress = monConfig.GetMonitoringAddress();
        ui32 threadsCount = monConfig.GetMonitoringThreads();
        Monitoring = CreateMonitoringService(monPort, monAddress, threadsCount);
    } else {
        Monitoring = CreateMonitoringServiceStub();
    }
}

void TBootstrap::InitLocalService()
{
    InitDbgConfigs();
    InitRdmaClient();
    InitSpdk();
    InitProfileLog();

    DiscoveryService = CreateDiscoveryServiceStub(
        FQDNHostName(),
        Configs->DiscoveryConfig->GetConductorInstancePort(),
        Configs->DiscoveryConfig->GetConductorSecureInstancePort()
    );

    const auto& config = Configs->ServerConfig->GetLocalServiceConfig()
        ? *Configs->ServerConfig->GetLocalServiceConfig()
        : NProto::TLocalServiceConfig();

    FileIOService = CreateAIOService();

    NvmeManager = CreateNvmeManager(
        Configs->DiskAgentConfig->GetSecureEraseTimeout());

    Service = CreateLocalService(
        config,
        DiscoveryService,
        CreateAioStorageProvider(
            FileIOService,
            NvmeManager,
            false   // directIO
        ));
}

void TBootstrap::InitNullService()
{
    InitDbgConfigs();
    InitRdmaClient();
    InitSpdk();
    InitProfileLog();

    const auto& config = Configs->ServerConfig->GetNullServiceConfig()
        ? *Configs->ServerConfig->GetNullServiceConfig()
        : NProto::TNullServiceConfig();

    Service = CreateNullService(config);
}

void TBootstrap::InitLWTrace()
{
    auto& probes = NLwTraceMonPage::ProbeRegistry();
    probes.AddProbesList(LWTRACE_GET_PROBES(BLOCKSTORE_SERVER_PROVIDER));
    probes.AddProbesList(LWTRACE_GET_PROBES(BLOCKSTORE_STORAGE_PROVIDER));
    probes.AddProbesList(LWTRACE_GET_PROBES(LWTRACE_INTERNAL_PROVIDER));
    probes.AddProbesList(LWTRACE_GET_PROBES(BLOBSTORAGE_PROVIDER));
    probes.AddProbesList(LWTRACE_GET_PROBES(TABLET_FLAT_PROVIDER));

    if (Configs->DiskAgentConfig->GetEnabled()) {
        probes.AddProbesList(LWTRACE_GET_PROBES(BLOCKSTORE_DISK_AGENT_PROVIDER));
    }

    auto diagnosticsConfig = Configs->DiagnosticsConfig;
    auto& lwManager = NLwTraceMonPage::TraceManager(diagnosticsConfig->GetUnsafeLWTrace());

    const TVector<std::tuple<TString, TString>> desc = {
        {"RequestStarted",                  "BLOCKSTORE_SERVER_PROVIDER"},
        {"BackgroundTaskStarted_Partition", "BLOCKSTORE_STORAGE_PROVIDER"},
        {"RequestReceived_DiskAgent",       "BLOCKSTORE_STORAGE_PROVIDER"},
    };

    auto traceLog = CreateUnifiedAgentLoggingService(
        Logging,
        diagnosticsConfig->GetTracesUnifiedAgentEndpoint(),
        diagnosticsConfig->GetTracesSyslogIdentifier()
    );

    if (auto samplingRate = diagnosticsConfig->GetSamplingRate()) {
        NLWTrace::TQuery query = ProbabilisticQuery(
            desc,
            samplingRate,
            diagnosticsConfig->GetLWTraceShuttleCount());
        lwManager.New(TraceLoggerId, query);
        TraceReaders.push_back(CreateTraceLogger(
            TraceLoggerId,
            traceLog,
            "BLOCKSTORE_TRACE"
        ));
    }

    if (auto samplingRate = diagnosticsConfig->GetSlowRequestSamplingRate()) {
        NLWTrace::TQuery query = ProbabilisticQuery(
            desc,
            samplingRate,
            diagnosticsConfig->GetLWTraceShuttleCount());
        lwManager.New(SlowRequestsFilterId, query);
        TraceReaders.push_back(CreateSlowRequestsFilter(
            SlowRequestsFilterId,
            traceLog,
            "BLOCKSTORE_TRACE",
            diagnosticsConfig->GetHDDSlowRequestThreshold(),
            diagnosticsConfig->GetSSDSlowRequestThreshold(),
            diagnosticsConfig->GetNonReplicatedSSDSlowRequestThreshold()));
    }

    lwManager.RegisterCustomAction(
        "ServiceErrorAction", &CreateServiceErrorActionExecutor);

    if (diagnosticsConfig->GetLWTraceDebugInitializationQuery()) {
        NLWTrace::TQuery query;
        ParseProtoTextFromFile(
            diagnosticsConfig->GetLWTraceDebugInitializationQuery(),
            query);

        lwManager.New("diagnostics", query);
    }
}

void TBootstrap::Start()
{
#define START_COMPONENT(c)                                                     \
    if (c) {                                                                   \
        c->Start();                                                            \
        STORAGE_INFO("Started " << #c);                                        \
    }                                                                          \
// START_COMPONENT

    START_COMPONENT(AsyncLogger);
    START_COMPONENT(Logging);
    START_COMPONENT(LogbrokerService);
    START_COMPONENT(NotifyService);
    START_COMPONENT(Monitoring);
    START_COMPONENT(ProfileLog);
    START_COMPONENT(CgroupStatsFetcher);
    START_COMPONENT(DiscoveryService);
    START_COMPONENT(TraceProcessor);
    START_COMPONENT(TraceSerializer);
    START_COMPONENT(ClientPercentiles);
    START_COMPONENT(StatsAggregator);
    START_COMPONENT(YdbStorage);
    START_COMPONENT(StatsUploader);
    START_COMPONENT(Spdk);
    START_COMPONENT(ActorSystem);
    START_COMPONENT(SubmissionQueue);
    START_COMPONENT(CompletionQueue);
    START_COMPONENT(FileIOService);
    START_COMPONENT(Service);
    START_COMPONENT(VhostServer);
    START_COMPONENT(NbdServer);
    START_COMPONENT(GrpcEndpointListener);
    START_COMPONENT(Executor);
    START_COMPONENT(Server);
    START_COMPONENT(ServerStatsUpdater);
    START_COMPONENT(BackgroundThreadPool);
    START_COMPONENT(RdmaClient);

    // we need to start scheduler after all other components for 2 reasons:
    // 1) any component can schedule a task that uses a dependency that hasn't
    // started yet
    // 2) we have loops in our dependencies, so there is no 'correct' starting
    // order
    START_COMPONENT(Scheduler);

    if (Configs->Options->MemLock) {
        LockProcessMemory(Log);
        STORAGE_INFO("Process memory locked");
    }

    EndpointService->RestoreEndpoints();
    STORAGE_INFO("Started endpoints restoring");

#undef START_COMPONENT
}

void TBootstrap::Stop()
{
#define STOP_COMPONENT(c)                                                      \
    if (c) {                                                                   \
        c->Stop();                                                             \
        STORAGE_INFO("Stopped " << #c);                                        \
    }                                                                          \
// STOP_COMPONENT

    // stopping scheduler before all other components to avoid races between
    // scheduled tasks and shutting down of component dependencies
    STOP_COMPONENT(Scheduler);

    STOP_COMPONENT(RdmaClient);
    STOP_COMPONENT(BackgroundThreadPool);
    STOP_COMPONENT(ServerStatsUpdater);
    STOP_COMPONENT(Server);
    STOP_COMPONENT(Executor);
    STOP_COMPONENT(GrpcEndpointListener);
    STOP_COMPONENT(NbdServer);
    STOP_COMPONENT(VhostServer);
    STOP_COMPONENT(Service);
    STOP_COMPONENT(FileIOService);
    STOP_COMPONENT(CompletionQueue);
    STOP_COMPONENT(SubmissionQueue);
    STOP_COMPONENT(ActorSystem);
    STOP_COMPONENT(Spdk);
    STOP_COMPONENT(StatsUploader);
    STOP_COMPONENT(YdbStorage);
    STOP_COMPONENT(StatsAggregator);
    STOP_COMPONENT(ClientPercentiles);
    STOP_COMPONENT(TraceSerializer);
    STOP_COMPONENT(TraceProcessor);
    STOP_COMPONENT(DiscoveryService);
    STOP_COMPONENT(CgroupStatsFetcher);
    STOP_COMPONENT(ProfileLog);
    STOP_COMPONENT(Monitoring);
    STOP_COMPONENT(LogbrokerService);
    STOP_COMPONENT(Logging);
    STOP_COMPONENT(AsyncLogger);

#undef STOP_COMPONENT
}

TProgramShouldContinue& TBootstrap::GetShouldContinue()
{
    return ActorSystem
        ? ActorSystem->GetProgramShouldContinue()
        : ShouldContinue;
}

IBlockStorePtr TBootstrap::GetBlockStoreService()
{
    return Service;
}

}   // namespace NCloud::NBlockStore::NServer
