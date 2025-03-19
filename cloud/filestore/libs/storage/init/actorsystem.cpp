#include "actorsystem.h"

#include <cloud/filestore/libs/diagnostics/request_stats.h>
#include <cloud/filestore/libs/diagnostics/storage_counters.h>
#include <cloud/filestore/libs/storage/api/components.h>
#include <cloud/filestore/libs/storage/api/service.h>
#include <cloud/filestore/libs/storage/api/ss_proxy.h>
#include <cloud/filestore/libs/storage/api/tablet.h>
#include <cloud/filestore/libs/storage/api/tablet_proxy.h>
#include <cloud/filestore/libs/storage/core/config.h>
#include <cloud/filestore/libs/storage/service/service.h>
#include <cloud/filestore/libs/storage/ss_proxy/ss_proxy.h>
#include <cloud/filestore/libs/storage/tablet/tablet.h>
#include <cloud/filestore/libs/storage/tablet_proxy/tablet_proxy.h>

#include <cloud/storage/core/libs/api/hive_proxy.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/hive_proxy/hive_proxy.h>
#include <cloud/storage/core/libs/kikimr/actorsystem.h>

#include <kikimr/yndx/security/ticket_parser.h>

#include <ydb/core/base/blobstorage.h>
#include <ydb/core/mind/labels_maintainer.h>
#include <ydb/core/mind/local.h>
#include <ydb/core/mind/tenant_pool.h>
#include <ydb/core/mon/mon.h>
#include <ydb/core/protos/config.pb.h>
#include <ydb/core/tablet/node_tablet_monitor.h>
#include <ydb/core/tablet/tablet_list_renderer.h>
#include <ydb/core/driver_lib/run/kikimr_services_initializers.h>
#include <ydb/core/driver_lib/run/run.h>

namespace NCloud::NFileStore::NStorage {

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
    const TActorSystemArgs Args;
    IRequestStatsRegistryPtr StatsRegistry;

public:
    TStorageServicesInitializer(
            const TActorSystemArgs& args,
            IRequestStatsRegistryPtr statsRegistry)
        : Args{args}
        , StatsRegistry{std::move(statsRegistry)}
    {}

    void InitializeServices(
        TActorSystemSetup* setup,
        const TAppData* appData) override
    {
        //
        // StorageService
        //

        auto indexService = CreateStorageService(
            Args.StorageConfig,
            StatsRegistry);

        setup->LocalServices.emplace_back(
            MakeStorageServiceId(),
            TActorSetupCmd(
                indexService.release(),
                TMailboxType::Revolving,
                appData->UserPoolId));

        //
        // IndexTabletProxy
        //

        auto tabletProxy = CreateIndexTabletProxy(Args.StorageConfig);

        setup->LocalServices.emplace_back(
            MakeIndexTabletProxyServiceId(),
            TActorSetupCmd(
                tabletProxy.release(),
                TMailboxType::Revolving,
                appData->UserPoolId));

        //
        // SSProxy
        //

        auto ssProxy = CreateSSProxy(Args.StorageConfig);

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
            TDuration::Seconds(1),  // HiveLockExpireTimeout, does not matter
            TFileStoreComponents::HIVE_PROXY,
            Args.StorageConfig->GetTabletBootInfoCacheFilePath(),
            Args.StorageConfig->GetHiveProxyFallbackMode()
        }, nullptr);

        setup->LocalServices.emplace_back(
            MakeHiveProxyServiceId(),
            TActorSetupCmd(
                hiveProxy.release(),
                TMailboxType::Revolving,
                appData->UserPoolId));
    }
};

////////////////////////////////////////////////////////////////////////////////

class TCustomLocalServiceInitializer final
    : public IServiceInitializer
{
private:
    const TActorSystemArgs Args;
    IStorageCountersPtr StorageCounters;

public:
    TCustomLocalServiceInitializer(
            const TActorSystemArgs& args,
            IStorageCountersPtr counters)
        : Args(args)
        , StorageCounters(counters)
    {}

    void InitializeServices(
        TActorSystemSetup* setup,
        const TAppData* appData) override
    {
        auto config = Args.StorageConfig;
        auto profileLog = Args.ProfileLog;
        auto storageCounters = StorageCounters;

        auto tabletFactory = [=] (const TActorId& owner, TTabletStorageInfo* storage) {
            Y_VERIFY(storage->TabletType == TTabletTypes::FileStore);
            auto actor = CreateIndexTablet(
                owner,
                storage,
                config,
                profileLog,
                storageCounters);
            return actor.release();
        };

        TLocalConfig::TPtr localConfig = new TLocalConfig();

        TTabletTypes::EType tabletType = TTabletTypes::FileStore;
        if (config->GetDisableLocalService()) {
            // prevent tablet start via tablet types filter inside local service
            // empty filter == all, so configure invalid tablet type to prevent any
            // it allows to properly register in system and not to break things e.g. Viewer
            tabletType = TTabletTypes::TypeInvalid;
        }

        localConfig->TabletClassInfo[tabletType] =
            TLocalConfig::TTabletClassInfo(
                new TTabletSetupInfo(
                    tabletFactory,
                    TMailboxType::ReadAsFilled,
                    appData->UserPoolId,
                    TMailboxType::ReadAsFilled,
                    appData->SystemPoolId));

        TTenantPoolConfig::TPtr tenantPoolConfig = new TTenantPoolConfig(localConfig);
        tenantPoolConfig->AddStaticSlot(Args.StorageConfig->GetSchemeShardDir());

        setup->LocalServices.emplace_back(
            MakeTenantPoolRootID(),
            TActorSetupCmd(
                CreateTenantPool(tenantPoolConfig),
                TMailboxType::ReadAsFilled,
                appData->SystemPoolId));

        setup->LocalServices.emplace_back(
            TActorId(),
            TActorSetupCmd(
                CreateLabelsMaintainer(Args.AppConfig->GetMonitoringConfig()),
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

    // in case KiKiMR monitoring not configured
    TIntrusivePtr<TIndexMonPage> IndexMonPage = new TIndexMonPage("", "");

public:
    TActorSystem(const TActorSystemArgs& args)
        : TKikimrRunner(std::make_shared<TModuleFactories>())
        , Args(args)
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
        TFileStoreComponents::START,
        TFileStoreComponents::END,
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
    servicesMask.EnableTabletMonitor = 1;
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

    auto counters = CreateStorageCounters(AppData->Counters);
    auto statsRegistry = CreateRequestStatsRegistry(
        "service",
        {},
        FILESTORE_COUNTERS_ROOT(AppData->Counters),
        CreateWallClockTimer());
    auto services = CreateServiceInitializersList(runConfig, servicesMask);
    services->AddServiceInitializer(new TStorageServicesInitializer(Args, std::move(statsRegistry)));
    services->AddServiceInitializer(new TCustomLocalServiceInitializer(Args, counters));

    InitializeActorSystem(runConfig, services, servicesMask);
}

void TActorSystem::Start()
{
    KikimrStart();
}

void TActorSystem::Stop()
{
    KikimrStop(false);
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

}   // namespace NCloud::NFileStore::NStorage
