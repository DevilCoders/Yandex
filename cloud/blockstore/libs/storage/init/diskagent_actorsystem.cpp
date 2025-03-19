#include "diskagent_actorsystem.h"

#include <cloud/blockstore/libs/kikimr/components.h>
#include <cloud/blockstore/libs/storage/api/disk_agent.h>
#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/disk_registry_proxy.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/ss_proxy.h>
#include <cloud/blockstore/libs/storage/api/undelivered.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/disk_agent/config.h>
#include <cloud/blockstore/libs/storage/disk_agent/disk_agent.h>
#include <cloud/blockstore/libs/storage/disk_registry/disk_registry.h>
#include <cloud/blockstore/libs/storage/disk_registry/disk_registry_actor.h>
#include <cloud/blockstore/libs/storage/disk_registry_proxy/config.h>
#include <cloud/blockstore/libs/storage/disk_registry_proxy/disk_registry_proxy.h>
#include <cloud/blockstore/libs/storage/service/service.h>
#include <cloud/blockstore/libs/storage/ss_proxy/ss_proxy.h>
#include <cloud/blockstore/libs/storage/undelivered/undelivered.h>
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

class TStorageServicesInitializer final
    : public IServiceInitializer
{
private:
    const TDiskAgentActorSystemArgs Args;

public:
    TStorageServicesInitializer(const TDiskAgentActorSystemArgs& args)
        : Args(args)
    {}

    void InitializeServices(
        TActorSystemSetup* setup,
        const TAppData* appData) override
    {
        Args.StorageConfig->Register(*appData->Icb);

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
    }
};

////////////////////////////////////////////////////////////////////////////////

class TActorSystem final
    : public TKikimrRunner
    , public IActorSystem
{
private:
    const TDiskAgentActorSystemArgs Args;

    bool Running;

    // in case KiKiMR monitoring not configured
    TIntrusivePtr<TIndexMonPage> IndexMonPage = new TIndexMonPage("", "");

public:
    TActorSystem(const TDiskAgentActorSystemArgs& args)
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

    if (Args.AppConfig->HasAuthConfig()) {
        servicesMask.EnableSecurityServices = 1;
    }

#if defined(ACTORSLIB_COLLECT_EXEC_STATS)
    servicesMask.EnableStatsCollector = 1;
#endif

    auto services = CreateServiceInitializersList(runConfig, servicesMask);

    // TODO:
    // services->AddServiceInitializer(new TTabletMonitorInitializer(
    //     runConfig,
    //     MakeIntrusive<TCustomTabletStateClassifier>(),
    //     MakeIntrusive<TCustomTabletListRenderer>()));

    services->AddServiceInitializer(new TStorageServicesInitializer(Args));

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

IActorSystemPtr CreateDiskAgentActorSystem(const TDiskAgentActorSystemArgs& args)
{
    auto actorSystem = MakeIntrusive<TActorSystem>(args);
    actorSystem->Init();
    return actorSystem;
}

}   // namespace NCloud::NBlockStore::NStorage
