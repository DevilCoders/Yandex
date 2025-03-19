#include "actorsystem.h"

#include <cloud/blockstore/libs/kikimr/components.h>
#include <cloud/blockstore/libs/storage/api/authorizer.h>
#include <cloud/blockstore/libs/storage/api/disk_agent.h>
#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/disk_registry_proxy.h>
#include <cloud/blockstore/libs/storage/api/metering.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/stats_service.h>
#include <cloud/blockstore/libs/storage/api/ss_proxy.h>
#include <cloud/blockstore/libs/storage/api/undelivered.h>
#include <cloud/blockstore/libs/storage/api/user_stats.h>
#include <cloud/blockstore/libs/storage/api/volume_balancer.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/auth/authorizer.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/disk_agent/config.h>
#include <cloud/blockstore/libs/storage/disk_agent/disk_agent.h>
#include <cloud/blockstore/libs/storage/disk_registry/disk_registry.h>
#include <cloud/blockstore/libs/storage/disk_registry/disk_registry_actor.h>
#include <cloud/blockstore/libs/storage/disk_registry_proxy/config.h>
#include <cloud/blockstore/libs/storage/disk_registry_proxy/disk_registry_proxy.h>
#include <cloud/blockstore/libs/storage/metering/metering.h>
#include <cloud/blockstore/libs/storage/partition/part_actor.h>
#include <cloud/blockstore/libs/storage/partition2/part2_actor.h>
#include <cloud/blockstore/libs/storage/service/service.h>
#include <cloud/blockstore/libs/storage/stats_service/stats_service.h>
#include <cloud/blockstore/libs/storage/ss_proxy/ss_proxy.h>
#include <cloud/blockstore/libs/storage/undelivered/undelivered.h>
#include <cloud/blockstore/libs/storage/user_stats/user_stats.h>
#include <cloud/blockstore/libs/storage/volume/volume.h>
#include <cloud/blockstore/libs/storage/volume/volume_actor.h>
#include <cloud/blockstore/libs/storage/volume_balancer/volume_balancer.h>
#include <cloud/blockstore/libs/storage/volume_proxy/volume_proxy.h>
#include <cloud/storage/core/libs/api/hive_proxy.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>
#include <cloud/storage/core/libs/hive_proxy/hive_proxy.h>
#include <cloud/storage/core/libs/kikimr/actorsystem.h>

#include <kikimr/yndx/security/ticket_parser.h>

#include <ydb/core/base/blobstorage.h>
#include <ydb/core/blobstorage/testload/test_load_actor.h>
#include <ydb/core/mind/labels_maintainer.h>
#include <ydb/core/mind/local.h>
#include <ydb/core/mind/tenant_pool.h>
#include <ydb/core/mon/mon.h>
#include <ydb/core/protos/config.pb.h>
#include <ydb/core/tablet/node_tablet_monitor.h>
#include <ydb/core/tablet/tablet_list_renderer.h>
#include <ydb/core/driver_lib/run/kikimr_services_initializers.h>
#include <ydb/core/driver_lib/run/run.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NMonitoring;

using namespace NKikimr;
using namespace NKikimr::NKikimrServicesInitializers;
using namespace NKikimr::NNodeTabletMonitor;

using namespace NCloud::NStorage;

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TCustomTabletStateClassifier final
    : public TTabletStateClassifier
{
};

////////////////////////////////////////////////////////////////////////////////

struct TCustomTabletListRenderer final
    : public TTabletListRenderer
{
    TString GetUserStateName(const TTabletListElement& elem) override
    {
        if (elem.TabletStateInfo->HasUserState()) {
            ui32 state = elem.TabletStateInfo->GetUserState();
            switch (elem.TabletStateInfo->GetType()) {
                case TTabletTypes::BlockStoreVolume:
                    return TVolumeActor::GetStateName(state);
                case TTabletTypes::BlockStorePartition:
                    return NPartition::TPartitionActor::GetStateName(state);
                case TTabletTypes::BlockStorePartition2:
                    return NPartition2::TPartitionActor::GetStateName(state);
                case TTabletTypes::BlockStoreDiskRegistry:
                    return TDiskRegistryActor::GetStateName(state);
                default:
                    break;
            }
        }
        return TTabletListRenderer::GetUserStateName(elem);
    }
};

////////////////////////////////////////////////////////////////////////////////

class TStorageServicesInitializer final
    : public IServiceInitializer
{
private:
    const TActorSystemArgs Args;

public:
    TStorageServicesInitializer(const TActorSystemArgs& args)
        : Args(args)
    {}

    void InitializeServices(
        TActorSystemSetup* setup,
        const TAppData* appData) override
    {
        Args.StorageConfig->Register(*appData->Icb);

        //
        // SSProxy
        //

        auto ssProxy = CreateSSProxy(Args.StorageConfig, Args.FileIOService);

        setup->LocalServices.emplace_back(
            MakeSSProxyServiceId(),
            TActorSetupCmd(
                ssProxy.release(),
                TMailboxType::Revolving,
                appData->UserPoolId));

        //
        // HiveProxy
        //

        auto hiveProxy = CreateHiveProxy({
            Args.StorageConfig->GetPipeClientRetryCount(),
            Args.StorageConfig->GetPipeClientMinRetryTime(),
            Args.StorageConfig->GetHiveLockExpireTimeout(),
            TBlockStoreComponents::HIVE_PROXY,
            Args.StorageConfig->GetTabletBootInfoCacheFilePath(),
            Args.StorageConfig->GetHiveProxyFallbackMode(),
        }, Args.FileIOService);

        setup->LocalServices.emplace_back(
            MakeHiveProxyServiceId(),
            TActorSetupCmd(
                hiveProxy.release(),
                TMailboxType::Revolving,
                appData->UserPoolId));

        //
        // VolumeProxy
        //

        auto volumeProxy = CreateVolumeProxy(
            Args.StorageConfig,
            Args.TraceSerializer);

        setup->LocalServices.emplace_back(
            MakeVolumeProxyServiceId(),
            TActorSetupCmd(
                volumeProxy.release(),
                TMailboxType::Revolving,
                appData->UserPoolId));

        //
        // DiskRegistryProxy
        //

        auto diskRegistryProxy = CreateDiskRegistryProxy(
            Args.StorageConfig,
            Args.DiskRegistryProxyConfig);

        setup->LocalServices.emplace_back(
            MakeDiskRegistryProxyServiceId(),
            TActorSetupCmd(
                diskRegistryProxy.release(),
                TMailboxType::Revolving,
                appData->UserPoolId));

        //
        // StorageStatsService
        //

        auto storageStatsService = CreateStorageStatsService(
            Args.StorageConfig,
            Args.DiagnosticsConfig,
            Args.StatsUploader,
            Args.StatsAggregator);

        setup->LocalServices.emplace_back(
            MakeStorageStatsServiceId(),
            TActorSetupCmd(
                storageStatsService.release(),
                TMailboxType::Revolving,
                appData->BatchPoolId));

        //
        // StorageService
        //

        auto storageService = CreateStorageService(
            Args.StorageConfig,
            Args.DiagnosticsConfig,
            Args.ProfileLog,
            Args.BlockDigestGenerator,
            Args.DiscoveryService,
            Args.TraceSerializer,
            Args.RdmaClient,
            Args.VolumeStats);

        setup->LocalServices.emplace_back(
            MakeStorageServiceId(),
            TActorSetupCmd(
                storageService.release(),
                TMailboxType::Revolving,
                appData->UserPoolId));

        //
        // StorageUserStats
        //

        auto storageUserStats = NUserStats::CreateStorageUserStats(
            Args.VolumeStats);

        setup->LocalServices.emplace_back(
            MakeStorageUserStatsId(),
            TActorSetupCmd(
                storageUserStats.release(),
                TMailboxType::Revolving,
                appData->BatchPoolId));

        //
        // MeteringWriter
        //

        if (Args.StorageConfig->GetMeteringFilename()) {
            auto meteringService = CreateMeteringWriter(
                std::make_unique<TFileLogBackend>(
                    Args.StorageConfig->GetMeteringFilename()));

            setup->LocalServices.emplace_back(
                MakeMeteringWriterId(),
                TActorSetupCmd(
                    meteringService.release(),
                    TMailboxType::HTSwap,
                    appData->IOPoolId));
        }

        //
        // UndeliveredHandler
        //

        auto undeliveredHandler = CreateUndeliveredHandler();

        setup->LocalServices.emplace_back(
            MakeUndeliveredHandlerServiceId(),
            TActorSetupCmd(
                undeliveredHandler.release(),
                TMailboxType::Revolving,
                appData->UserPoolId));

        //
        // Authorizer
        //

        auto authorizer = CreateAuthorizerActor(
            Args.StorageConfig,
            Args.AppConfig->HasAuthConfig());

        setup->LocalServices.emplace_back(
            MakeAuthorizerServiceId(),
            TActorSetupCmd(
                authorizer.release(),
                TMailboxType::Revolving,
                appData->UserPoolId));

        //
        // DiskAgent
        //

        if (Args.DiskAgentConfig->GetEnabled()) {
            auto diskAgent = CreateDiskAgent(
                Args.StorageConfig,
                Args.DiskAgentConfig,
                Args.Spdk,
                Args.Allocator,
                Args.AioStorageProvider,
                Args.ProfileLog,
                Args.BlockDigestGenerator,
                Args.Logging,
                Args.RdmaServer);

            setup->LocalServices.emplace_back(
                MakeDiskAgentServiceId(Args.NodeId),
                TActorSetupCmd(
                    diskAgent.release(),
                    TMailboxType::Revolving,
                    appData->UserPoolId));
        }

        //
        // VolumeBindingService
        //

        auto volumeBalancerService = CreateVolumeBalancerActor(
            Args.AppConfig,
            Args.StorageConfig,
            Args.VolumeStats,
            Args.CgroupStatsFetcher,
            MakeStorageServiceId());

        setup->LocalServices.emplace_back(
            MakeVolumeBalancerServiceId(),
            TActorSetupCmd(
                volumeBalancerService.release(),
                TMailboxType::Revolving,
                appData->UserPoolId));

        //
        // BlobStorage LoadActorService
        //

        if (Args.StorageConfig->GetEnableLoadActor()) {
            IActorPtr loadActorService(CreateTestLoadActor(appData->Counters));

            setup->LocalServices.emplace_back(
                MakeBlobStorageLoadID(Args.NodeId),
                TActorSetupCmd(
                    loadActorService.release(),
                    TMailboxType::HTSwap,
                    appData->UserPoolId));
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class TCustomLocalServiceInitializer final
    : public IServiceInitializer
{
private:
    const NKikimrConfig::TAppConfig& AppConfig;
    const TStorageConfigPtr StorageConfig;
    const TDiagnosticsConfigPtr DiagnosticsConfig;
    const IProfileLogPtr ProfileLog;
    const IBlockDigestGeneratorPtr BlockDigestGenerator;
    const ITraceSerializerPtr TraceSerializer;
    const NLogbroker::IServicePtr LogbrokerService;
    const NNotify::IServicePtr NotifyService;
    const NRdma::IClientPtr RdmaClient;

public:
    TCustomLocalServiceInitializer(
            const NKikimrConfig::TAppConfig& appConfig,
            TStorageConfigPtr storageConfig,
            TDiagnosticsConfigPtr diagnosticsConfig,
            IProfileLogPtr profileLog,
            IBlockDigestGeneratorPtr blockDigestGenerator,
            ITraceSerializerPtr traceSerializer,
            NLogbroker::IServicePtr logbrokerService,
            NNotify::IServicePtr notifyService,
            NRdma::IClientPtr rdmaClient)
        : AppConfig(appConfig)
        , StorageConfig(std::move(storageConfig))
        , DiagnosticsConfig(std::move(diagnosticsConfig))
        , ProfileLog(std::move(profileLog))
        , BlockDigestGenerator(std::move(blockDigestGenerator))
        , TraceSerializer(std::move(traceSerializer))
        , LogbrokerService(std::move(logbrokerService))
        , NotifyService(std::move(notifyService))
        , RdmaClient(std::move(rdmaClient))
    {}

    void InitializeServices(
        TActorSystemSetup* setup,
        const TAppData* appData) override
    {
        auto storageConfig = StorageConfig;
        auto diagnosticsConfig = DiagnosticsConfig;
        auto profileLog = ProfileLog;
        auto blockDigestGenerator = BlockDigestGenerator;
        auto traceSerializer = TraceSerializer;
        auto logbrokerService = LogbrokerService;
        auto notifyService = NotifyService;
        auto rdmaClient = RdmaClient;

        auto volumeFactory = [=] (const TActorId& owner, TTabletStorageInfo* storage) {
            Y_VERIFY(storage->TabletType == TTabletTypes::BlockStoreVolume);

            auto actor = CreateVolumeTablet(
                owner,
                storage,
                storageConfig,
                diagnosticsConfig,
                profileLog,
                blockDigestGenerator,
                traceSerializer,
                rdmaClient
            );
            return actor.release();
        };

        auto diskRegistryFactory = [=] (const TActorId& owner, TTabletStorageInfo* storage) {
            Y_VERIFY(storage->TabletType == TTabletTypes::BlockStoreDiskRegistry);

            auto tablet = CreateDiskRegistry(
                owner,
                storage,
                storageConfig,
                diagnosticsConfig,
                logbrokerService,
                notifyService);
            return tablet.release();
        };

        if (!StorageConfig->GetDisableLocalService()) {
            TLocalConfig::TPtr localConfig = new TLocalConfig();
            localConfig->TabletClassInfo[TTabletTypes::BlockStoreVolume] =
                TLocalConfig::TTabletClassInfo(
                    new TTabletSetupInfo(
                        volumeFactory,
                        TMailboxType::ReadAsFilled,
                        appData->UserPoolId,
                        TMailboxType::ReadAsFilled,
                        appData->SystemPoolId));

            localConfig->TabletClassInfo[TTabletTypes::BlockStoreDiskRegistry] =
                TLocalConfig::TTabletClassInfo(
                    new TTabletSetupInfo(
                        diskRegistryFactory,
                        TMailboxType::ReadAsFilled,
                        appData->UserPoolId,
                        TMailboxType::ReadAsFilled,
                        appData->SystemPoolId));

            TTenantPoolConfig::TPtr tenantPoolConfig = new TTenantPoolConfig(localConfig);
            tenantPoolConfig->AddStaticSlot(StorageConfig->GetSchemeShardDir());

            setup->LocalServices.emplace_back(
                MakeTenantPoolRootID(),
                TActorSetupCmd(
                    CreateTenantPool(tenantPoolConfig),
                    TMailboxType::ReadAsFilled,
                    appData->SystemPoolId));
        }

        setup->LocalServices.emplace_back(
            TActorId(),
            TActorSetupCmd(
                CreateLabelsMaintainer(AppConfig.GetMonitoringConfig()),
                TMailboxType::ReadAsFilled,
                appData->SystemPoolId));
    }
};

////////////////////////////////////////////////////////////////////////////////

class TActorSystem final
    : public TKikimrRunner
    , public IActorSystem
{
private:
    const TActorSystemArgs Args;

    bool Running;

    // in case KiKiMR monitoring not configured
    TIntrusivePtr<TIndexMonPage> IndexMonPage = new TIndexMonPage("", "");

public:
    TActorSystem(const TActorSystemArgs& args)
        : TKikimrRunner(std::make_shared<TModuleFactories>())
        , Args(args)
        , Running(false)
    {}

    void Init();

    void Start() override;
    void Stop() override;

    TActorId Register(IActorPtr actor, TStringBuf executorName) override;
    bool Send(const TActorId& recipient, IEventBasePtr event) override;

    TLog CreateLog(const TString& component) override;

    IMonPagePtr RegisterIndexPage(const TString& path, const TString& title) override;
    void RegisterMonPage(IMonPagePtr page) override;
    IMonPagePtr GetMonPage(const TString& path) override;
    TDynamicCountersPtr GetCounters() override;

    TProgramShouldContinue& GetProgramShouldContinue() override;

private:
    void InitializeLocalModules(const TKikimrRunConfig& config);
};

////////////////////////////////////////////////////////////////////////////////

void TActorSystem::InitializeLocalModules(const TKikimrRunConfig& config)
{
    Y_UNUSED(config);

    ModuleFactories->CreateTicketParser = NKikimrYandex::CreateTicketParser;
}

////////////////////////////////////////////////////////////////////////////////

void TActorSystem::Init()
{
    TKikimrRunConfig runConfig(
        *Args.AppConfig,
        Args.NodeId,
        TKikimrScopeId(Args.ScopeId));

    runConfig.AppConfig.MutableMonitoringConfig()->SetRedirectMainPageTo("");

    InitializeRegistries(runConfig);
    InitializeMonitoring(runConfig);
    InitializeAppData(runConfig);
    InitializeLogSettings(runConfig);
    InitializeLocalModules(runConfig);

    LogSettings->Append(
        TBlockStoreComponents::START,
        TBlockStoreComponents::END,
        GetComponentName);

    TBasicKikimrServicesMask servicesMask;
    servicesMask.DisableAll();
    servicesMask.EnableBasicServices = 1;
    servicesMask.EnableLogger = 1;
    servicesMask.EnableSchedulerActor = 1;
    servicesMask.EnableProfiler = 1;
    servicesMask.EnableSelfPing = 1;
    servicesMask.EnableRestartsCountPublisher = 1;
    servicesMask.EnableStateStorageService = 1;
    servicesMask.EnableTabletResolver = 1;
    servicesMask.EnableTabletMonitor = 0;   // configured manually
    servicesMask.EnableTabletMonitoringProxy = 1;
    servicesMask.EnableTabletCountersAggregator = 1;
    servicesMask.EnableBSNodeWarden = 1;
    servicesMask.EnableWhiteBoard = 1;
    servicesMask.EnableResourceBroker = 1;
    servicesMask.EnableSharedCache = 1;
    servicesMask.EnableTxProxy = 1;
    servicesMask.EnableIcbService = 1;
    servicesMask.EnableLocalService = 0;    // configured manually
    servicesMask.EnableNodeIdentifier = 1;
    servicesMask.EnableSchemeBoardMonitoring = 1;

    if (Args.AppConfig->HasAuthConfig()) {
        servicesMask.EnableSecurityServices = 1;
    }

#if defined(ACTORSLIB_COLLECT_EXEC_STATS)
    servicesMask.EnableStatsCollector = 1;
#endif

    auto services = CreateServiceInitializersList(runConfig, servicesMask);

    services->AddServiceInitializer(new TTabletMonitorInitializer(
        runConfig,
        MakeIntrusive<TCustomTabletStateClassifier>(),
        MakeIntrusive<TCustomTabletListRenderer>()));

    services->AddServiceInitializer(new TStorageServicesInitializer(Args));

    services->AddServiceInitializer(new TCustomLocalServiceInitializer(
        *Args.AppConfig,
        Args.StorageConfig,
        Args.DiagnosticsConfig,
        Args.ProfileLog,
        Args.BlockDigestGenerator,
        Args.TraceSerializer,
        Args.LogbrokerService,
        Args.NotifyService,
        Args.RdmaClient));

    InitializeActorSystem(runConfig, services, servicesMask);
}

void TActorSystem::Start()
{
    KikimrStart();
    Running = true;
}

void TActorSystem::Stop()
{
    if (Running) {
        Send(MakeDiskAgentServiceId(Args.NodeId),
            std::make_unique<TEvents::TEvPoisonPill>());

        KikimrStop(false);
    }
}

TActorId TActorSystem::Register(IActorPtr actor, TStringBuf executorName)
{
    // TODO
    Y_UNUSED(executorName);

    return ActorSystem->Register(
        actor.release(),
        TMailboxType::Simple,
        AppData->UserPoolId);
}

bool TActorSystem::Send(const TActorId& recipient, IEventBasePtr event)
{
    return ActorSystem->Send(recipient, event.release());
}

TLog TActorSystem::CreateLog(const TString& componentName)
{
    if (LogBackend) {
        TLogSettings logSettings;
        logSettings.UseLocalTimestamps = LogSettings->UseLocalTimestamps;
        logSettings.SuppressNewLine = true;   // kikimr will do it for us

        auto component = LogSettings->FindComponent(componentName);
        if (component != NLog::InvalidComponent) {
            auto settings = LogSettings->GetComponentSettings(component);
            logSettings.FiltrationLevel = static_cast<ELogPriority>(settings.Raw.X.Level);
        } else {
            logSettings.FiltrationLevel = static_cast<ELogPriority>(LogSettings->DefPriority);
        }

        return CreateComponentLog(componentName, LogBackend, Args.AsyncLogger, logSettings);
    }

    return {};
}

IMonPagePtr TActorSystem::RegisterIndexPage(
    const TString& path,
    const TString& title)
{
    if (Monitoring) {
        return Monitoring->RegisterIndexPage(path, title);
    } else {
        return IndexMonPage->RegisterIndexPage(path, title);
    }
}

void TActorSystem::RegisterMonPage(IMonPagePtr page)
{
    if (Monitoring) {
        Monitoring->Register(page.Release());
    } else {
        IndexMonPage->Register(page.Release());
    }
}

IMonPagePtr TActorSystem::GetMonPage(const TString& path)
{
    if (Monitoring) {
        return Monitoring->FindPage(path);
    } else {
        return IndexMonPage->FindPage(path);
    }
}

TDynamicCountersPtr TActorSystem::GetCounters()
{
    return Counters;
}

TProgramShouldContinue& TActorSystem::GetProgramShouldContinue()
{
    return TKikimrRunner::KikimrShouldContinue;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IActorSystemPtr CreateActorSystem(const TActorSystemArgs& args)
{
    auto actorSystem = MakeIntrusive<TActorSystem>(args);
    actorSystem->Init();
    return actorSystem;
}

}   // namespace NCloud::NBlockStore::NStorage
