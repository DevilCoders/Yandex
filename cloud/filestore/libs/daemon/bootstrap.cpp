#include "bootstrap.h"

#include <cloud/filestore/libs/diagnostics/config.h>
#include <cloud/filestore/libs/diagnostics/profile_log.h>
#include <cloud/filestore/libs/diagnostics/request_stats.h>
#include <cloud/filestore/libs/server/probes.h>
#include <cloud/filestore/libs/server/server.h>
#include <cloud/filestore/libs/storage/init/actorsystem.h>
#include <cloud/filestore/libs/storage/init/node.h>
#include <cloud/filestore/libs/storage/core/config.h>

#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/task_queue.h>
#include <cloud/storage/core/libs/common/thread_pool.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>
#include <cloud/storage/core/libs/diagnostics/trace_processor.h>
#include <cloud/storage/core/libs/daemon/mlock.h>
#include <cloud/storage/core/libs/kikimr/actorsystem.h>

#include <ydb/core/protos/config.pb.h>

#include <library/cpp/lwtrace/mon/mon_lwtrace.h>
#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/stream/file.h>
#include <util/system/fs.h>

namespace NCloud::NFileStore::NDaemon {

using namespace NActors;
using namespace NKikimr;
using namespace NMonitoring;

namespace
{

////////////////////////////////////////////////////////////////////////////////

template <typename T>
void ParseProtoTextFromFile(const TString& fileName, T& dst)
{
    TFileInput in(fileName);
    ParseFromTextFormat(in, dst);
}

////////////////////////////////////////////////////////////////////////////////

ui32 GetAndUpdateRestartsCount(const TString& fileName)
{
    ui32 restartsCount = 0;
    if (NFs::Exists(fileName)) {
        TFileInput in(fileName);
        restartsCount = FromStringWithDefault<ui32>(in.ReadAll(), 0);
    }
    TFileOutput out(fileName);
    out.Write(ToString(restartsCount + 1));
    return restartsCount;
}

////////////////////////////////////////////////////////////////////////////////

class TLoggingProxy final
    : public ILoggingService
{
private:
    IActorSystemPtr ActorSystem;

public:
    TLoggingProxy(IActorSystemPtr actorSystem)
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
    TMonitoringProxy(IActorSystemPtr actorSystem)
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

} // namespace

////////////////////////////////////////////////////////////////////////////////

TBootstrap::TBootstrap(
        const TBootstrapOptionsPtr& options,
        const TString& logComponent,
        const TString& metricsComponent)
    : MetricsComponent{metricsComponent}
    , Options{options}
{
    InitBootstrapLog(logComponent);
}

TBootstrap::~TBootstrap()
{}

void TBootstrap::Init()
{
    InitDiagnosticsConfig();

    Timer = CreateWallClockTimer();
    Scheduler = CreateScheduler();
    BackgroundThreadPool = CreateThreadPool("Background", 1);
    BackgroundScheduler = CreateBackgroundScheduler(
        Scheduler,
        BackgroundThreadPool);

    if (Options->ProfileFile) {
        ProfileLog = CreateProfileLog(
            {
                Options->ProfileFile,
                DiagnosticsConfig->GetProfileLogTimeThreshold()
            },
            Timer,
            BackgroundScheduler);
    } else {
        ProfileLog = CreateProfileLogStub();
    }

    STORAGE_INFO("ProfileLog initialized");

    if (TraceReaders.size()) {
        TraceProcessor = CreateTraceProcessor(
            Timer,
            BackgroundScheduler,
            Logging,
            Monitoring,
            "NFS_TRACE",
            NLwTraceMonPage::TraceManager(false),
            TraceReaders);

        STORAGE_INFO("TraceProcessor initialized");
    }

    if (Options->Service == EServiceKind::Kikimr) {
        InitActorSystem();
    }
    InitDiagnostics();

    Server = CreateServer();

    RequestStatsUpdater = CreateStatsUpdater(
        Timer,
        BackgroundScheduler,
        StatsRegistry,
        GetIncompleteRequestProviders());

    STORAGE_INFO("RequestStatsUpdater initialized");

    STORAGE_INFO("init completed");
}

void TBootstrap::Start()
{
    StartComponents();

    if (!Options->NoMemLock) {
        LockProcessMemory(Log);
        STORAGE_INFO("Process memory locked");
    }
}

void TBootstrap::Stop()
{
    StopComponents();
}

TProgramShouldContinue& TBootstrap::GetProgramShouldContinue()
{
    if (ActorSystem) {
        return ActorSystem->GetProgramShouldContinue();
    }

    return ProgramShouldContinue;
}

void TBootstrap::InitBootstrapLog(const TString& logComponent)
{
    TLogSettings logSettings;
    logSettings.UseLocalTimestamps = true;
    BootstrapLogging = CreateLoggingService("console", logSettings);
    BootstrapLogging->Start();
    Log = BootstrapLogging->CreateLog(logComponent);
}

void TBootstrap::InitDiagnosticsConfig()
{
    NProto::TDiagnosticsConfig config;

    if (Options->DiagnosticsConfig) {
        ParseProtoTextFromFile(Options->DiagnosticsConfig, config);
    }

    DiagnosticsConfig = std::make_shared<TDiagnosticsConfig>(std::move(config));
}

void TBootstrap::InitDiagnostics()
{
    if (ActorSystem) {
        Logging = std::make_shared<TLoggingProxy>(ActorSystem);
        Monitoring = std::make_shared<TMonitoringProxy>(ActorSystem);
    } else {
        TLogSettings logSettings;
        logSettings.UseLocalTimestamps = true;

        if (Options->VerboseLevel) {
            auto level = GetLogLevel(Options->VerboseLevel);
            Y_ENSURE(level, "unknown log level: " << Options->VerboseLevel.Quote());

            logSettings.FiltrationLevel = *level;
        }

        Logging = CreateLoggingService("console", logSettings);

        if (Options->MonitoringPort) {
            Monitoring = CreateMonitoringService(
                Options->MonitoringPort,
                Options->MonitoringAddress,
                Options->MonitoringThreads);
        } else {
            Monitoring = CreateMonitoringServiceStub();
        }

    }

    InitLWTrace();

    STORAGE_INFO("LWTrace initialized");

    StatsRegistry = CreateRequestStatsRegistry(
        MetricsComponent,
        DiagnosticsConfig,
        FILESTORE_COUNTERS_ROOT(Monitoring->GetCounters()),
        Timer);

    auto serverStats = StatsRegistry->GetServerStats();
    if (Options->RestartsCountFile) {
        serverStats->UpdateRestartsCount(GetAndUpdateRestartsCount(Options->RestartsCountFile));
    }

    STORAGE_INFO("Stats initialized");
}

void TBootstrap::InitKikimrConfigs()
{
    KikimrConfig = std::make_shared<NKikimrConfig::TAppConfig>();

    auto& logConfig = *KikimrConfig->MutableLogConfig();
    if (Options->VerboseLevel) {
        auto level = GetLogLevel(Options->VerboseLevel);
        Y_ENSURE(level, "unknown log level: " << Options->VerboseLevel.Quote());

        logConfig.SetDefaultLevel(*level);
    }

    if (Options->LogConfig) {
        ParseProtoTextFromFile(Options->LogConfig, logConfig);
    }

    auto& monConfig = *KikimrConfig->MutableMonitoringConfig();
    if (Options->MonitoringConfig) {
        ParseProtoTextFromFile(Options->MonitoringConfig, monConfig);
    }

    if (Options->MonitoringAddress) {
        monConfig.SetMonitoringAddress(Options->MonitoringAddress);
    }
    if (Options->MonitoringPort) {
        monConfig.SetMonitoringPort(Options->MonitoringPort);
    }
    if (Options->MonitoringThreads) {
        monConfig.SetMonitoringThreads(Options->MonitoringThreads);
    }
    if (!monConfig.HasMonitoringThreads()) {
        monConfig.SetMonitoringThreads(1);  // reasonable defaults
    }

    auto& restartsCountConfig = *KikimrConfig->MutableRestartsCountConfig();
    if (Options->RestartsCountFile) {
        restartsCountConfig.SetRestartsCountFile(Options->RestartsCountFile);
    }

    auto& sysConfig = *KikimrConfig->MutableActorSystemConfig();
    if (Options->SysConfig) {
        ParseProtoTextFromFile(Options->SysConfig, sysConfig);
    }

    auto& interconnectConfig = *KikimrConfig->MutableInterconnectConfig();
    if (Options->InterconnectConfig) {
        ParseProtoTextFromFile(Options->InterconnectConfig, interconnectConfig);
    }

    interconnectConfig.SetStartTcp(true);

    auto& domainsConfig = *KikimrConfig->MutableDomainsConfig();
    if (Options->DomainsConfig) {
        ParseProtoTextFromFile(Options->DomainsConfig, domainsConfig);
    }

    auto& nameServiceConfig = *KikimrConfig->MutableNameserviceConfig();
    if (Options->NameServiceConfig) {
        ParseProtoTextFromFile(Options->NameServiceConfig, nameServiceConfig);
    }

    if (Options->SuppressVersionCheck) {
        nameServiceConfig.SetSuppressVersionCheck(true);
    }

    auto& dynamicNameServiceConfig = *KikimrConfig->MutableDynamicNameserviceConfig();
    if (Options->DynamicNameServiceConfig) {
        ParseProtoTextFromFile(Options->DynamicNameServiceConfig, dynamicNameServiceConfig);
    }

    if (Options->AuthConfig) {
        auto& authConfig = *KikimrConfig->MutableAuthConfig();
        ParseProtoTextFromFile(Options->AuthConfig, authConfig);
    }

    auto* bs = KikimrConfig->MutableBlobStorageConfig();
    bs->MutableServiceSet()->AddAvailabilityDomains(1);
}

void TBootstrap::InitStorageConfig()
{
    NProto::TStorageConfig config;
    if (Options->StorageConfig) {
        ParseProtoTextFromFile(Options->StorageConfig, config);
    }

    if (Options->SchemeShardDir) {
        config.SetSchemeShardDir(Options->SchemeShardDir);
    }

    StorageConfig = std::make_shared<NStorage::TStorageConfig>(config);
}


void TBootstrap::InitActorSystem()
{
    InitKikimrConfigs();
    InitStorageConfig();

    // TODO trace serializer

    NStorage::TRegisterDynamicNodeOptions registerOpts;
    registerOpts.Domain = Options->Domain;
    registerOpts.SchemeShardDir = StorageConfig->GetSchemeShardDir();
    registerOpts.NodeBrokerAddress = Options->NodeBrokerAddress;
    registerOpts.NodeBrokerPort = Options->NodeBrokerPort;
    registerOpts.InterconnectPort = Options->InterconnectPort;

    auto [nodeId, scopeId] = NStorage::RegisterDynamicNode(
        KikimrConfig,
        registerOpts,
        Log);

    NStorage::TActorSystemArgs args;
    args.NodeId = nodeId;
    args.ScopeId = scopeId;
    args.AppConfig = KikimrConfig;
    args.StorageConfig = StorageConfig;
    args.ProfileLog = ProfileLog;

    ActorSystem = NStorage::CreateActorSystem(args);
}


} // namespace NCloud::NFileStore::NDaemon