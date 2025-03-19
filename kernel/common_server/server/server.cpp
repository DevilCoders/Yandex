#include "server.h"

#include <kernel/common_server/api/localization/localization.h>
#include <kernel/common_server/api/security/security.h>
#include <kernel/common_server/roles/actions/common.h>
#include <kernel/common_server/util/datacenter.h>
#include <library/cpp/mediator/messenger.h>
#include <library/cpp/tvmauth/client/facade.h>
#include <library/cpp/config/extra/yconf.h>
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/library/unistat/log/backend.h>
#include <kernel/common_server/library/unistat/signals.h>
#include <kernel/common_server/processors/rt_background/handler.h>
#include <kernel/common_server/processors/notifiers/handler.h>
#include <kernel/common_server/processors/user_auth/user_auth.h>
#include <kernel/common_server/processors/user_role/user_role.h>
#include <kernel/common_server/processors/roles/item.h>
#include <kernel/common_server/processors/roles/permissions.h>
#include <kernel/common_server/processors/roles/role.h>
#include <kernel/common_server/processors/tags/description.h>
#include <kernel/common_server/processors/settings/handler.h>
#include <kernel/common_server/processors/obfuscator/handler.h>
#include <kernel/common_server/processors/db_migrations/migrations.h>
#include <kernel/common_server/proposition/object.h>
#include <kernel/common_server/processors/proposition/handler.h>
#include <kernel/common_server/obfuscator/object.h>
#include <kernel/common_server/settings/abstract/object.h>
#include <kernel/common_server/tags/abstract.h>
#include <kernel/common_server/roles/abstract/role.h>
#include <kernel/common_server/roles/abstract/item.h>
#include <kernel/common_server/user_role/abstract/abstract.h>
#include <kernel/common_server/user_auth/abstract/abstract.h>
#include <kernel/common_server/notifications/manager.h>
#include <kernel/common_server/rt_background/settings.h>
#include <kernel/common_server/util/types/string_normal.h>
#include <kernel/common_server/auth/common/manager.h>

class TBaseServer::TRequestHandlersMtpQueue : public TCountedMtpQueue {
public:
    TRequestHandlersMtpQueue(const TString& name)
        : Name(name)
    {
    }

    void DoSignals() {
        TCSSignals::LSignal("threads-count", GetBusyThreadsCount())("kind", "busy")("pool", Name);
        TCSSignals::LSignal("threads-count", GetFreeThreadsCount())("kind", "free")("pool", Name);
        TCSSignals::LSignal("pool_counters", GetBusyThreadsCount())("kind", "threads_busy")("pool", Name);
        TCSSignals::LSignal("pool_counters", GetFreeThreadsCount())("kind", "threads_free")("pool", Name);
        TCSSignals::LSignal("pool_counters", Size())("kind", "queue_size")("pool", Name);
    }

private:
    const TString Name;
};

class TBaseServer::TPQConstructionContext: public NCS::IPQConstructionContext {
private:
    const IBaseServer& Server;
public:
    TPQConstructionContext(const IBaseServer& server)
        : Server(server)
    {}
    virtual NStorage::IDatabase::TPtr GetDatabase(const TString& dbName) const override {
        return Server.GetDatabase(dbName);
    }
    virtual const NCS::IReadSettings& GetReadSettings() const override {
        return Server.GetSettings();
    }

    virtual const NCS::ITvmManager* GetTvmManager() const override {
        return Server.GetTvmManager();
    }

    virtual NExternalAPI::TSender::TPtr GetSenderPtr(const TString& serviceId) const override {
        return Server.GetSenderPtr(serviceId);
    }

    virtual NCS::IElSignature::TPtr GetElSignature(const TString& signatureId) const override {
        return Server.GetElSignature(signatureId);
    }
};

class TBaseServer::TDBConstructionContext : public NCS::NStorage::TBalancingDatabaseConstructionContext {
private:
    const IBaseServer& Server;
public:
    TDBConstructionContext(const IBaseServer& server)
        : Server(server)
    {}

    virtual const NCS::ITvmManager* GetTvmManager() const override {
        return Server.GetTvmManager();
    }
};

class TBaseServer::THandlersActualizationThread: public IObjectInQueue {
private:
    const TBaseServer* Server = nullptr;
public:
    THandlersActualizationThread(const TBaseServer* server)
        : Server(server) {

    }

    void Process(void* /*threadSpecificResource*/) override {
        while (AtomicGet(Server->IsActive)) {
            Server->RefreshProcessorsInfo();
            Sleep(TDuration::Seconds(2));
        }
    }

};

class TBaseServer::THandlerThreadsPoolMonitoring: public IObjectInQueue {
private:
    const TBaseServer* Server = nullptr;
public:
    THandlerThreadsPoolMonitoring(const TBaseServer* server)
        : Server(server) {

    }

    void Process(void* /*threadSpecificResource*/) override {
        while (AtomicGet(Server->IsActive)) {
            if (Server->HttpServer) {
                TCSSignals::LSignal("threads-count", Server->HttpServer->GetBusyThreadsCount())("kind", "busy")("pool", "http-server");
                TCSSignals::LSignal("threads-count", Server->HttpServer->GetFreeThreadsCount())("kind", "free")("pool", "http-server");
                TCSSignals::LSignal("pool_counters", Server->HttpServer->Size())("kind", "queue_size")("pool", "http-server");
            }
            for (auto&& [_, handler] : Server->RequestHandlers) {
                handler->DoSignals();
            }
            Sleep(TDuration::Seconds(2));
        }
    }

};

void TBaseServer::LoggingNotify(const TString& notifierId, const NCS::NLogging::TBaseLogRecord& r) const {
    TAtomicSharedPtr<IFrontendNotifier> notifier = GetNotifier(!!notifierId ? notifierId : "internal-alerts-notifier");
    IFrontendNotifier::Notify(notifier, r.SerializeToString(NCS::NLogging::ELogRecordFormat::HR));
}

class TReadSettingsWrapper: public NFrontend::IReadSettings {
private:
    const TBaseServer& Server;
public:
    TReadSettingsWrapper(const TBaseServer& server)
        : Server(server) {

    }
    virtual bool GetValueStr(const TString& key, TString& result) const override {
        if (!Server.HasSettings() || !Server.GetIsActive()) {
            result = "";
            return false;
        } else {
            return Server.GetSettings().GetValueStr(key, result);
        }
    }
};

TBaseServer::TBaseServer(const TConfig& config)
    : Config(config)
{
    SafeSettings = MakeAtomicShared<TReadSettingsWrapper>(*this);
    TLoggerOperator<TGlobalLog>::Log().ResetBackend(MakeHolder<TUnistatLogBackend>(TLoggerOperator<TGlobalLog>::Log().ReleaseBackend()));
    TLoggerOperator<TFLEventLog>::Set(new TFLEventLog);
    TLoggerOperator<TFLEventLog>::Get()->ResetBackend(Config.GetEventLog().CreateLogBackend());
    if (Config.GetEventLog().GetPriority() != LOG_MAX_PRIORITY) {
        TLoggerOperator<TFLEventLog>::Get()->UsePriorityLimit(Config.GetEventLog().GetPriority());
    }
    if (Config.GetEventLog().IsBackgroundUsage()) {
        TLoggerOperator<TFLEventLog>::Get()->ActivateBackgroundWriting(Config.GetEventLog().GetQueueSizeLimit(), Config.GetEventLog().GetSkipOnQueueLimitExceeded());
    }
    TFLEventLog::SetFormat(Config.GetEventLog().GetFormat());
    PQConstructionContext = MakeHolder<TPQConstructionContext>(*this);
    RegisterGlobalMessageProcessor(this);
}

TBaseServer::~TBaseServer() {
    UnregisterGlobalMessageProcessor(this);
    CHECK_WITH_LOG(AtomicGet(IsActive) == 0);
}

const NCS::NPropositions::TDBManager* TBaseServer::GetPropositionsManager() const {
    return PropositionsManager.Get();
}

NCS::NObfuscator::TObfuscatorManagerContainer TBaseServer::GetObfuscatorManager() const {
    return ObfuscatorManager;
}

const NCS::IPQConstructionContext& TBaseServer::GetPQConstructionContext() const {
    CHECK_WITH_LOG(PQConstructionContext);
    return *PQConstructionContext;
}

NFrontend::TConstantsInfoReport TBaseServer::BuildInterfaceConstructor(TAtomicSharedPtr<IUserPermissions> permissions) const {
    NFrontend::TConstantsInfoReport cReport;
    if (permissions->Has<TRTBackgroundPermissions>()) {
        cReport.Register("background").InitScheme<TRTBackgroundProcessContainer>(*this).SetTitle("_title").SetObjectId("bp_name");
    }
    if (permissions->Has<TNotifierPermissions>()) {
        cReport.Register("notifier").InitScheme<TNotifierContainer>(*this).SetTitle("name");
    }
    if (permissions->Has<NCS::TUserAuthPermissions>()) {
        cReport.Register("user_auth").InitScheme<TAuthUserLink>(*this).InitSearchScheme<TAuthUserLink>(*this).SetTitle("_title").SetObjectId("link_id").SetSearchSchemeRequired(true);
    }
    if (permissions->Has<NCS::TUserRolePermissions>()) {
        cReport.Register("user_role").InitScheme<TUserRolesCompiled>(*this).InitSearchScheme<TUserRolesCompiled>(*this).SetTitle("system_user_id").SetSearchSchemeRequired(true);
    }
    if (permissions->Has<NCS::TActionPermissions>()) {
        cReport.Register("action").InitScheme<TItemPermissionContainer>(*this).SetTitle("item_id").SetObjectId("item_id");
    }
    if (permissions->Has<NCS::TRolePermissions>()) {
        cReport.Register("role").InitScheme<TUserRoleInfo>(*this).SetTitle("role_name");
    }
    if (permissions->Has<NCS::NHandlers::TTagDescriptionPermissions>()) {
        cReport.Register("tag_description").InitScheme<TDBTagDescription>(*this).SetTitle("name");
    }
    if (permissions->Has<TSettingsPermissions>()) {
        cReport.Register("settings").InitScheme<NFrontend::TSetting>(*this).SetTitle("setting_key");
    }
    if (permissions->Has<NCS::NHandlers::TObfuscatorPermissions>()) {
        cReport.Register("obfuscator").InitScheme<NCS::NObfuscator::TDBObfuscator>(*this).SetTitle("name").SetObjectId("obfuscator_id");
    }
    if (permissions->Has<NCS::NHandlers::TDBMigrationsPermissions>()) {
        auto& c = cReport.Register("db_migrations").InitScheme<NCS::NStorage::TDBMigration>(*this).SetTitle("_title").SetObjectId("id");
        if (permissions->Check<NCS::NHandlers::TDBMigrationsPermissions>(TAdministrativePermissions::EObjectAction::Modify)) {
            c.AddActivity("Перенакатить", "apply").AddActivity("Отметить накатанным", "mark_apply");
        }
    }
    if (permissions->Has<NCS::NHandlers::TPropositionPermissions>()) {
        cReport.Register("proposition_history").InitScheme<TObjectEvent<NCS::NPropositions::TDBProposition>>(*this).InitSearchScheme<TObjectEvent<NCS::NPropositions::TDBProposition>>(*this).SetTitle("_title_event").SetSearchSchemeRequired(true);
        cReport.Register("proposition_verdicts_history").InitScheme<TObjectEvent<NCS::NPropositions::TDBVerdict>>(*this).InitSearchScheme<TObjectEvent<NCS::NPropositions::TDBVerdict>>(*this).SetTitle("_title_event").SetSearchSchemeRequired(true);
        cReport.Register("proposition").InitScheme<NCS::NPropositions::TDBProposition>(*this).SetTitle("_title").SetObjectId("proposition_id");
        cReport.Register("proposition_verdict")
            .SetScheme(NCS::NHandlers::TPropositionVerdictInfoHandler::GetExtendedPropositionScheme(*this))
            .SetTitle("_title")
            .SetObjectId("proposition_id");
    }
    InitConstants(cReport, permissions);
    return cReport;
}

TVector<NCS::NScheme::THandlerScheme> TBaseServer::BuildHandlersScheme(const TSet<TString>& endpoints) const {
    TVector<NCS::NScheme::THandlerScheme> result;
    TMap<NCS::TPathHandlerInfo, IRequestProcessorConfig::TPtr> localHandlers;
    {
        TReadGuard rg(MutexHandlerInfoSwitch);
        localHandlers = HandlerPrefixesInfo;
    }
    for (auto&& i : localHandlers) {
        bool correct = endpoints.empty();
        for (auto&& checker : endpoints) {
            if (checker.EndsWith("*")) {
                correct = i.first.GetPathPatternCorrected().StartsWith(checker.substr(0, checker.size() - 1));
            } else {
                correct = checker == i.first.GetPathPatternCorrected();
            }
            if (correct) {
                break;
            }
        }
        if (!correct) {
            continue;
        }
        auto handler = i.second->ConstructProcessor(nullptr, this, {});
        if (!handler) {
            continue;
        }
        TMaybe<NCS::NScheme::THandlerScheme> scheme = handler->BuildScheme(i.first, *this);
        if (!scheme) {
            continue;
        }
        for (auto&& i : scheme->GetPath()) {
            if (!i.GetValue() && i.GetVariable()) {
                scheme->MutableParameters().AddPath<TFSString>(i.GetVariable()).SetRequired(true);
            }
        }
        result.emplace_back(std::move(*scheme));
    }
    return result;
}

TMaybe<NSimpleMeta::TConfig> TBaseServer::GetRequestConfig(const TString& apiName, const NNeh::THttpRequest& request) const {
    const TString& configVal = GetSettings().GetValueDef<TString>(
        { "clients." + apiName + ".reask_configs." + request.GetUri(), "clients." + apiName + ".reask_configs.*", "clients.*.reask_configs." + request.GetUri(), "clients.reask_configs." + request.GetUri() },
        "", "");
    if (!configVal) {
        return Nothing();
    }
    TAnyYandexConfig config;
    if (!config.ParseMemory(configVal)) {
        ERROR_LOG << "Cannot parse client config for " << request.GetUri() << Endl;
        return Nothing();
    }
    NSimpleMeta::TConfig result;
    result.InitFromSection(config.GetRootSection());
    return result;
}

bool TBaseServer::Process(IMessage* message_) {
    if (auto message = message_->As<TCollectDaemonInfoMessage>()) {
        message->SetCType(Config.DaemonConfig.GetCType());
        message->SetService(Config.DaemonConfig.GetService());
        return true;
    }
    return false;
}


TAtomicSharedPtr<IFrontendNotifier> TBaseServer::GetNotifier(const TString& name) const {
    auto it = Notifications.find(name);
    if (it != Notifications.end()) {
        return it->second;
    }
    if (NotifiersManager) {
        TMaybe<TNotifierContainer> container = NotifiersManager->GetObject(name);
        return container ? container->GetNotifierPtr() : nullptr;
    }
    return nullptr;
}

const IPermissionsManager& TBaseServer::GetPermissionsManager() const {
    return *PermissionsManager;
}

const NCS::IAbstractCipher* TBaseServer::GetCipher(const TString& cipherName) const {
    TFLEventLog::Warning("Deprecated: use GetCipherPtr instead");
    auto ptr = GetCipherPtr(cipherName);
    return ptr ? ptr.Get() : nullptr;
}

const NCS::TCertsManger& TBaseServer::GetCertsManger() const {
    return *CertsManager;
}

NCS::NStorage::TDBMigrationsManager::TPtr TBaseServer::GetDBMigrationsManager(const TString& dbName) const {
    if (auto manager = MapFindPtr(DBMigrationManagers, dbName)) {
        return *manager;
    }
    return NCS::NStorage::TDBMigrationsManager::TPtr();
}

IUserPermissions::TPtr TBaseServer::BuildPermissionFromItems(const TVector<TItemPermissionContainer>& items, const TString& userId) const {
    auto result = MakeHolder<TSystemUserPermissions>(userId);
    result->AddAbilities(items);
    return result.Release();
}

bool TBaseServer::RefreshProcessorsInfo() const {
    TMap<TString, IRequestProcessorConfig::TPtr> configuredHandlers = Config.GetServerProcessorsObjects();
    {
        TVector<NFrontend::TSetting> settings;
        if (!GetSettings().GetAllSettings(settings)) {
            TFLEventLog::Alert("cannot fetch settings");
            return false;
        }
        {
            const ui32 sizeCType = TString(GetCType() + ".handlers.").size();
            for (auto&& i : settings) {
                ui32 sizeSkip;
                if (i.GetKey().StartsWith("handlers.")) {
                    sizeSkip = 9;
                } else if (i.GetKey().StartsWith(GetCType() + ".handlers.")) {
                    sizeSkip = sizeCType;
                } else {
                    continue;
                }
                auto parts = SplitString(i.GetKey().substr(sizeSkip), ".");
                if (parts.size() != 1) {
                    continue;
                }
                const TString& handlerName = NCS::TStringNormalizer::TruncRet(parts.front(), '/');
                auto gLogging = TFLRecords::StartContext()("handler_name", handlerName);
                if (!!i.GetValue()) {
                    TAnyYandexConfig config;
                    if (!config.ParseMemory(i.GetValue())) {
                        TFLEventLog::Alert("cannot parse settings config");
                        continue;
                    }
                    const TYandexConfig::Section* section = config.GetRootSection();
                    if (!section) {
                        TFLEventLog::Alert("incorrect settings config root section");
                        continue;
                    }
                    const TString typeName = section->GetDirectives().Value<TString>("ProcessorType", handlerName);
                    IRequestProcessorConfig::TPtr handlerConfig = IRequestProcessorConfig::TFactory::Construct(typeName, handlerName);
                    if (!handlerConfig) {
                        TFLEventLog::Alert("incorrect handler type")("type", typeName);
                        continue;
                    }
                    if (!handlerConfig->Init(section)) {
                        TFLEventLog::Alert("cannot initialize handler");
                        continue;
                    }
                    if (!configuredHandlers.emplace(handlerName, handlerConfig).second) {
                        TFLEventLog::Alert("handler duplication");
                        continue;
                    }
                }
            }
        }
    }

    TMap<NCS::TPathHandlerInfo, IRequestProcessorConfig::TPtr> handlerPrefixes;
    for (auto&& i : configuredHandlers) {
        NCS::TPathHandlerInfo info;
        if (info.DeserializeFromString(i.first)) {
            handlerPrefixes.emplace(std::move(info), i.second);
        }
    }

    {
        TWriteGuard g(MutexHandlerInfoSwitch);
        std::swap(handlerPrefixes, HandlerPrefixesInfo);
    }

    return true;
}

TRequestProcessorConfigContainer TBaseServer::GetProcessorInfo(const TString& handlerName) const {
    IRequestProcessorConfig::TPtr result;
    ui32 templatesUsage = 0;
    ui32 deep = 0;
    TMap<TString, TString> params;
    TReadGuard g(MutexHandlerInfoSwitch);
    for (auto&& [pattern, configPtr] : HandlerPrefixesInfo) {
        TMap<TString, TString> paramsLocal;
        ui32 currentTemplatesUsage;
        ui32 currentDeep;
        if (!pattern.Match(handlerName, currentTemplatesUsage, currentDeep, paramsLocal)) {
            continue;
        }
        if (!!result) {
            if (currentTemplatesUsage > templatesUsage) {
                continue;
            } else if (templatesUsage == currentTemplatesUsage && deep > currentDeep) {
                continue;
            }
        }
        result = configPtr;
        deep = currentDeep;
        templatesUsage = currentTemplatesUsage;
        params = std::move(paramsLocal);
    }
    TRequestProcessorConfigContainer cResult(result);
    cResult.SetSelectionParams(std::move(params));
    if (!!result) {
        cResult.MutableSelectionParams().insert(result->GetDefaultUrlParams().begin(), result->GetDefaultUrlParams().end());
    }
    return cResult;
}

bool TBaseServer::HasProcessorInfo(const TString& handlerName) const {
    return !!GetProcessorInfo(handlerName);
}

IThreadPool* TBaseServer::GetRequestsHandler(const TString& handlerName) const {
    auto it = RequestHandlers.find(!!handlerName ? handlerName : "default");
    if (it == RequestHandlers.end()) {
        return nullptr;
    }
    return it->second.Get();
}

void TBaseServer::Run() {
    try {
        try {
            TStringStream serverConfig;
            NConfig::ReadYandexCfg(Config.ToString()).ToJson(serverConfig);
            INFO_LOG << "Actual config: " << serverConfig.Str() << Endl;
        } catch (...) {
            ERROR_LOG << "Failed to serialize server config" << Endl;   
        }

        CertsManager = MakeHolder<NCS::TCertsManger>(Config.GetCertsManagerConfig());
        CertsManager->Start();

        TCSSignals::RegisterMonitoring(Config.GetMonitoringConfig());
        NSimpleMeta::TConfig adc = NSimpleMeta::TConfig::ForRequester();

        if (Config.GetGeobaseConfig().IsEnabled()) {
            NGeobase::NImpl::TLookup::TInitTraits traits;
            if (Config.GetGeobaseConfig().IsLockMemory()) {
                traits.LockMemory(true);
            }
            if (Config.GetGeobaseConfig().IsPreloading()) {
                traits.Preloading(true);
            }
            auto geobase = MakeHolder<NGeobase::NImpl::TLookup>(Config.GetGeobaseConfig().GetPath(), traits);
            Geobase = MakeHolder<TGeobaseProvider>(std::move(geobase), Config.GetGeobaseConfig());
        } else {
            Geobase = MakeHolder<TFakeGeobaseProvider>(Config.GetGeobaseConfig());
        }

        for (const auto& [name, config]: Config.GetExternalDatabases()) {
            TDBConstructionContext context(*this);
            context.SetBalancingPolicy(new NCS::NStorage::TBalancingPolicyOperator(name, SafeSettings));
            context.SetDBName(name);
            auto db = config.ConstructDatabase(&context);
            Databases[name] = db;
            if (const auto* migConfig = MapFindPtr(Config.GetDBMigrationManagerConfigs(), name)) {
                auto migrationsManager = MakeAtomicShared<NCS::NStorage::TDBMigrationsManager>(THistoryContext(db), *migConfig, this);
                CHECK_WITH_LOG(migrationsManager->ApplyMigrations());
                migrationsManager->Start();
                DBMigrationManagers[name] = migrationsManager;
            }
        }
        LocksManager = Config.GetLocksManagerConfigContainer().BuildManager(*this);

        TMessageOnAfterDatabaseConstructed message(Databases);
        SendGlobalMessage(message);

        if (!!Config.GetEmulationsManagerConfig()) {
            EmulationsManager = Config.GetEmulationsManagerConfig()->BuildManager(*this);
        }

        if (!!Config.GetSnapshotsControllerConfig().GetDBName()) {
            SnapshotsController = MakeHolder<NCS::TSnapshotsController>(Config.GetSnapshotsControllerConfig(), *this);
        }

        for (auto&& i : Config.GetDatabases()) {
            Storages[i.first] = i.second.ConstructStorage();
        }

        if (Config.GetTvmConfigs()) {
            TvmManager = MakeHolder<NCS::TTvmManager>(Config.GetTvmConfigs());
        }

        if (Config.GetLocalizationConfig()) {
            auto it = Databases.find(Config.GetLocalizationConfig()->GetDBName());
            AssertCorrectConfig(it != Databases.end(), "Incorrect DBName for localization");
            auto localization = MakeHolder<NCS::NLocalization::TLocalizationDB>(MakeHolder<THistoryContext>(it->second), *Config.GetLocalizationConfig());
            Localization.Reset(localization.Release());
        } else {
            Localization = MakeHolder<TFakeLocalization>();
        }

        AssertCorrectConfig(!!Config.GetSettingsConfig(), "no settings info");
        Settings = Config.GetSettingsConfig()->Construct(*this);
        AssertCorrectConfig(!!Settings, "incorrect settings info");

        for (auto&& i : Config.GetHandlers()) {
            const TString& name = i.first;
            auto queue = MakeHolder<TRequestHandlersMtpQueue>(name);
            queue->Start(i.second.GetThreadsCount(), i.second.GetQueueSizeLimit());
            RequestHandlers.emplace(name, std::move(queue));
        }

        if (Config.GetNotifiersManagerConfig()) {
            auto it = Databases.find(Config.GetNotifiersManagerConfig()->GetDBName());
            AssertCorrectConfig(it != Databases.end(), "Incorrect DBName for notifiers manager");
            auto hContext = MakeHolder<THistoryContext>(it->second);
            NotifiersManager = MakeHolder<TNotifiersManager>(*this, std::move(hContext), *Config.GetNotifiersManagerConfig());
            NotifiersManager->Start();
        }



        if (!!Config.GetResourcesManagerConfig().GetDBName()) {
            auto it = Databases.find(Config.GetResourcesManagerConfig().GetDBName());
            AssertCorrectConfig(it != Databases.end(), "Incorrect DBName for tag resources manager");
            ResourcesManager = MakeHolder<NCS::NResources::TDBManager>(it->second, Config.GetResourcesManagerConfig());
        }

        if (!!SnapshotsController) {
            SnapshotsController->Start();
        }

        if (!!Config.GetPropositionsManagerConfig().GetDBName()) {
            auto it = Databases.find(Config.GetPropositionsManagerConfig().GetDBName());
            AssertCorrectConfig(it != Databases.end(), "Incorrect DBName for propositions manager");
            PropositionsManager = MakeHolder<NCS::NPropositions::TDBManager>(it->second, Config.GetPropositionsManagerConfig(), *this);
            PropositionsManager->Start();
        }

        if (!!Config.GetTagDescriptionsConfig().GetDBName()) {
            auto it = Databases.find(Config.GetTagDescriptionsConfig().GetDBName());
            AssertCorrectConfig(it != Databases.end(), "Incorrect DBName for tag descriptions manager");
            TagDescriptionsManager = MakeHolder<TTagDescriptionsManager>(MakeHolder<THistoryContext>(it->second), Config.GetTagDescriptionsConfig(), *this);
            TagDescriptionsManager->Start();

        }

        {
            AuthUsersManager = Config.GetAuthUsersManagerConfig()->BuildManager(*this);
            AssertCorrectConfig(!!AuthUsersManager, "Incorrect UsersManager");
            AuthUsersManager->StartManager();
        }

        for (auto&& i : Config.GetCiphers()) {
            auto cipher = i.second->Construct(this);
            CHECK_WITH_LOG(!!cipher) << "incorrect configuration for cipher " << i.first << Endl;
            Ciphers.emplace(i.first, cipher);
        }

        if (!!Config.GetObfuscatorManagerConfig().GetDBName()) {
            auto it = Databases.find(Config.GetObfuscatorManagerConfig().GetDBName());
            AssertCorrectConfig(it != Databases.end(), "Incorrect DBName for obfuscator manager");
            ObfuscatorManager = MakeAtomicShared<NCS::NObfuscator::TDBManager>(it->second, Config.GetObfuscatorManagerConfig());
            ObfuscatorManager->Start();
        } else {
            ObfuscatorManager = MakeAtomicShared<NCS::NObfuscator::TFakeObfuscatorManager>();
        }

        for (auto&& i : Config.GetAbstractExternalApiConfigs()) {
            AddService(i.first, i.second, this);
        }
        for (auto&& [_, c] : Config.GetElSignaturesConfig().GetConfigs()) {
            ElSignatures.emplace(c->GetId(), c->Construct(*this));
        }
        for (auto&& i : Config.GetExternalQueueConfigs()) {
            AddQueue(i.second, GetPQConstructionContext());
        }
        StartExternalServiceClients();

        if (!!Config.GetGeoAreasConfig()) {
            GeoAreas.Reset(Config.GetGeoAreasConfig()->BuildManager(*this));
            AssertCorrectConfig(!!GeoAreas, "Incorrect GeoAreas");
            GeoAreas->Start();
        }

        for (auto&& i : Config.GetNotifications()) {
            auto notification = i.second->Construct();
            if (const auto& clientId = notification->GetTvmClientName()) {
                notification->SetTvmClient(TvmManager->GetTvmClient(*clientId));
            }
            notification->Start(*this);
            Notifications.emplace(i.first, notification);
        }

        TFLEventLog::InitAlertsNotifier(this);

        RolesManager.Reset(Config.GetRolesManagerConfig()->BuildManager(*this));
        if (RolesManager) {
            RolesManager->Start();
        }
        PermissionsManager.Reset(Config.GetPermissionsManagerConfig()->BuildManager(*this));
        if (PermissionsManager) {
            PermissionsManager->Start();
        }
        PersonalDataStorage.Reset(Config.GetPersonalDataStorageConfig().BuildStorage(*this));

        for (auto&& i : Config.GetSecretManagerConfigs()) {
            auto sManager = i.second->Construct(*this);
            CHECK_WITH_LOG(!!sManager) << "incorrect configuration for secrets manager " << i.first << Endl;
            SecretManagers.emplace(i.first, sManager);
            sManager->SecretsStart();
        }

        DoRun();

        if (!!Config.GetRTBackgroundManagerConfig()) {
            RTBackgroundManager = MakeHolder<TRTBackgroundManager>(*this->GetAsPtrSafe<IBaseServer>(), *Config.GetRTBackgroundManagerConfig());
            RTBackgroundManager->Start();
        }

        for (auto&& i : Config.GetServerProcessorsObjects()) {
            i.second->CheckServerForProcessor(this);
        }

        SendGlobalMessage<TServerStartedMessage>();

        HttpServer = MakeHolder<TFrontendHttpServer>(Config, this);

        RefreshProcessorsInfo();
        CHECK_WITH_LOG(HttpServer->Start()) << "Failed to start http server: " << HttpServer->GetError();
        CHECK_WITH_LOG(AtomicCas(&IsActive, 1, 0));

        HandlersActualizationThreads.Start(1);
        HandlersActualizationThreads.SafeAddAndOwn(MakeHolder<THandlersActualizationThread>(this));
        MonitoringThreads.Start(1);
        MonitoringThreads.SafeAddAndOwn(MakeHolder<THandlerThreadsPoolMonitoring>(this));
    } catch (...) {
        S_FAIL_LOG << CurrentExceptionMessage() << Endl;
    }
}

void TBaseServer::Stop(ui32 rigidStopLevel, const TCgiParameters* cgiParams /*= nullptr*/) {
    CHECK_WITH_LOG(AtomicCas(&IsActive, 0, 1));

    HttpServer->Shutdown();
    HttpServer->Stop();

    if (!!SnapshotsController) {
        SnapshotsController->Stop();
    }

    if (!!GeoAreas) {
        GeoAreas->Stop();
    }

    if (!!RTBackgroundManager) {
        RTBackgroundManager->Stop();
    }

    if (RolesManager) {
        RolesManager->Stop();
    }

    if (PermissionsManager) {
        PermissionsManager->Stop();
    }

    DoStop(rigidStopLevel, cgiParams);

    StopExternalServiceClients();
    MonitoringThreads.Stop();
    HandlersActualizationThreads.Stop();

    TFLEventLog::DropAlertsNotifier();
    if (PropositionsManager) {
        PropositionsManager->Stop();
    }
    if (ObfuscatorManager) {
        ObfuscatorManager->Stop();
    }
    if (!!NotifiersManager) {
        NotifiersManager->Stop();
    }


    if (TagDescriptionsManager) {
        TagDescriptionsManager->Stop();
    }

    for (auto&& i : SecretManagers) {
        i.second->SecretsStop();
    }


    AuthUsersManager->StopManager();

    for (auto&& i : RequestHandlers) {
        i.second->Stop();
    }
    for (auto&& i : Notifications) {
        i.second->Stop();
    }

    for (auto& [name, manager]: DBMigrationManagers) {
        manager->Stop();
    }

    RequestHandlers.clear();
    Storages.clear();
    Notifications.clear();
    TCSSignals::UnregisterMonitoring();
    CertsManager->Stop();
}
