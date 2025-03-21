#include "bootstrap.h"

#include "config_initializer.h"
#include "options.h"

#include <cloud/blockstore/libs/common/caching_allocator.h>
#include <cloud/blockstore/libs/diagnostics/block_digest.h>
#include <cloud/blockstore/libs/diagnostics/config.h>
#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/diagnostics/fault_injection.h>
#include <cloud/blockstore/libs/diagnostics/probes.h>
#include <cloud/blockstore/libs/diagnostics/profile_log.h>
#include <cloud/blockstore/libs/diagnostics/request_stats.h>
#include <cloud/blockstore/libs/diagnostics/server_stats.h>
#include <cloud/blockstore/libs/diagnostics/stats_aggregator.h>
#include <cloud/blockstore/libs/diagnostics/volume_stats.h>
#include <cloud/blockstore/libs/nvme/nvme.h>
#include <cloud/blockstore/libs/rdma/server.h>
#include <cloud/blockstore/libs/server/config.h>
#include <cloud/blockstore/libs/service_local/storage_aio.h>
#include <cloud/blockstore/libs/service_local/storage_null.h>
#include <cloud/blockstore/libs/spdk/config.h>
#include <cloud/blockstore/libs/spdk/env.h>
#include <cloud/blockstore/libs/spdk/rdma.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/features_config.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/libs/storage/disk_agent/config.h>
#include <cloud/blockstore/libs/storage/disk_agent/probes.h>
#include <cloud/blockstore/libs/storage/disk_registry_proxy/config.h>
#include <cloud/blockstore/libs/storage/init/diskagent_actorsystem.h>
#include <cloud/blockstore/libs/storage/init/node.h>

#include <cloud/storage/core/libs/aio/service.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/file_io_service.h>
#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/daemon/mlock.h>
#include <cloud/storage/core/libs/diagnostics/critical_events.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>
#include <cloud/storage/core/libs/diagnostics/stats_updater.h>
#include <cloud/storage/core/libs/diagnostics/trace_processor.h>
#include <cloud/storage/core/libs/diagnostics/trace_serializer.h>
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

namespace {

////////////////////////////////////////////////////////////////////////////////

const TString TraceLoggerId = "st_trace_logger";
const TString SlowRequestsFilterId = "st_slow_requests_filter";

////////////////////////////////////////////////////////////////////////////////

template <typename T>
void ParseProtoTextFromString(const TString& text, T& dst)
{
    TStringInput in(text);
    ParseFromTextFormat(in, dst);
}

template <typename T>
void ParseProtoTextFromFile(const TString& fileName, T& dst)
{
    TFileInput in(fileName);
    ParseFromTextFormat(in, dst);
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

NRdma::EWaitMode Convert(NProto::EWaitMode mode)
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

NRdma::TServerConfigPtr CreateRdmaServerConfig(
    NStorage::TDiskAgentConfig& agent)
{
    auto target = agent.GetRdmaTarget();
    auto config = std::make_shared<NRdma::TServerConfig>();

    if (target.HasServer()) {
        auto server = target.GetServer();

        config->Backlog = server.GetBacklog();
        config->QueueSize = server.GetQueueSize();
        config->MaxBufferSize = server.GetMaxBufferSize();
        config->KeepAliveTimeout = TDuration::MilliSeconds(
            server.GetKeepAliveTimeout());
        config->WaitMode = Convert(server.GetWaitMode());
        config->PollerThreads = server.GetPollerThreads();
    }

    return config;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TBootstrap::TBootstrap(TOptionsPtr options)
    : Configs(new TConfigInitializer(std::move(options)))
{}

TBootstrap::~TBootstrap()
{}

void TBootstrap::Init()
{
    TLogSettings logSettings;
    logSettings.UseLocalTimestamps = true;

    BootstrapLogging = CreateLoggingService("console", logSettings);
    Log = BootstrapLogging->CreateLog("BLOCKSTORE_SERVER");
    STORAGE_INFO("NBS server version: " << GetFullVersionString());

    Timer = CreateWallClockTimer();
    Scheduler = CreateScheduler();

    InitKikimrService();

    STORAGE_INFO("Kikimr service initialized");

    auto diagnosticsConfig = Configs->DiagnosticsConfig;
    if (TraceReaders.size()) {
        TraceProcessor = CreateTraceProcessor(
            Timer,
            Scheduler,
            Logging,
            Monitoring,
            "BLOCKSTORE_TRACE",
            NLwTraceMonPage::TraceManager(diagnosticsConfig->GetUnsafeLWTrace()),
            TraceReaders);

        STORAGE_INFO("TraceProcessor initialized");
    }

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
            Scheduler
        );
    } else {
        ProfileLog = CreateProfileLogStub();
    }
}

void TBootstrap::InitRdmaServer(NStorage::TDiskAgentConfig& config)
{
    try {
        if (config.HasRdmaTarget()) {
            RdmaServer = NRdma::CreateServer(
                Logging,
                Monitoring,
                CreateRdmaServerConfig(config));

            STORAGE_INFO("RDMA server initialized");
        }
    } catch (...) {
        STORAGE_ERROR("Failed to initialize RDMA server: "
            << CurrentExceptionMessage().c_str());

        RdmaServer = nullptr;
        PostponedCriticalEvents.push_back("AppCriticalEvents/RdmaError");
    }
}

void TBootstrap::InitKikimrService()
{
    Configs->InitKikimrConfig();
    Configs->InitServerConfig();
    Configs->InitFeaturesConfig();
    Configs->InitStorageConfig();
    Configs->InitDiskRegistryProxyConfig();
    Configs->InitDiagnosticsConfig();
    Configs->InitSpdkEnvConfig();

    NStorage::TRegisterDynamicNodeOptions registerOpts;
    registerOpts.Domain = Configs->Options->Domain;
    registerOpts.SchemeShardDir = Configs->StorageConfig->GetSchemeShardDir();
    registerOpts.NodeType = Configs->Options->NodeType;
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

    if (Configs->DiagnosticsConfig->GetUseAsyncLogger()) {
        AsyncLogger = CreateAsyncLogger();

        STORAGE_INFO("AsyncLogger initialized");
    }

    if (auto& config = *Configs->DiskAgentConfig; config.GetEnabled()) {
        switch (config.GetBackend()) {
            case NProto::DISK_AGENT_BACKEND_SPDK:
                Spdk = NSpdk::CreateEnv(Configs->SpdkEnvConfig);

                STORAGE_INFO("Spdk backend initialized");
                break;

            case NProto::DISK_AGENT_BACKEND_AIO:
                FileIOService = CreateAIOService();
                NvmeManager = CreateNvmeManager(config.GetSecureEraseTimeout());

                AioStorageProvider = CreateAioStorageProvider(
                    FileIOService,
                    NvmeManager,
                    !config.GetDirectIoFlagDisabled());

                STORAGE_INFO("Aio backend initialized");
                break;
            case NProto::DISK_AGENT_BACKEND_NULL:
                AioStorageProvider = CreateNullStorageProvider();
                STORAGE_INFO("Null backend initialized");
                break;
        }

        InitRdmaServer(config);
    }

    Allocator = CreateCachingAllocator(
        Spdk ? Spdk->GetAllocator() : TDefaultAllocator::Instance(),
        Configs->DiskAgentConfig->GetPageSize(),
        Configs->DiskAgentConfig->GetMaxPageCount(),
        Configs->DiskAgentConfig->GetPageDropSize());

    STORAGE_INFO("Allocator initialized");

    InitProfileLog();

    STORAGE_INFO("ProfileLog initialized");

    if (Configs->StorageConfig->GetBlockDigestsEnabled()) {
        BlockDigestGenerator = CreateExt4BlockDigestGenerator(
            Configs->StorageConfig->GetDigestedBlocksPercentage());
    } else {
        BlockDigestGenerator = CreateBlockDigestGeneratorStub();
    }

    STORAGE_INFO("DigestGenerator initialized");

    NStorage::TDiskAgentActorSystemArgs args;
    args.NodeId = nodeId;
    args.ScopeId = scopeId;
    args.AppConfig = Configs->KikimrConfig;
    args.StorageConfig = Configs->StorageConfig;
    args.DiskAgentConfig = Configs->DiskAgentConfig;
    args.DiskRegistryProxyConfig = Configs->DiskRegistryProxyConfig;
    args.AsyncLogger = AsyncLogger;
    args.Spdk = Spdk;
    args.Allocator = Allocator;
    args.FileIOService = FileIOService;
    args.AioStorageProvider = AioStorageProvider;
    args.ProfileLog = ProfileLog;
    args.BlockDigestGenerator = BlockDigestGenerator;
    args.RdmaServer = RdmaServer;
    args.Logging = logging;

    ActorSystem = NStorage::CreateDiskAgentActorSystem(args);

    STORAGE_INFO("ActorSystem initialized");

    logging->Init(ActorSystem);
    monitoring->Init(ActorSystem);

    InitLWTrace();

    STORAGE_INFO("LWTrace initialized");

    NSpdk::InitLogging(Logging->CreateLog("BLOCKSTORE_SPDK"));
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
    START_COMPONENT(Monitoring);
    START_COMPONENT(ProfileLog);
    START_COMPONENT(TraceProcessor);
    START_COMPONENT(Spdk);
    START_COMPONENT(RdmaServer);
    START_COMPONENT(ActorSystem);
    START_COMPONENT(FileIOService);

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

    STOP_COMPONENT(FileIOService);
    STOP_COMPONENT(ActorSystem);
    STOP_COMPONENT(Spdk);
    STOP_COMPONENT(RdmaServer);
    STOP_COMPONENT(TraceProcessor);
    STOP_COMPONENT(ProfileLog);
    STOP_COMPONENT(Monitoring);
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

}   // namespace NCloud::NBlockStore::NServer
