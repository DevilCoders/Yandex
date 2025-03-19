#pragma once

#include "config.h"
#include "http_server.h"

#include <kernel/daemon/messages.h>
#include <kernel/daemon/base_controller.h>
#include <kernel/daemon/server.h>

#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/notifications/manager.h>
#include <kernel/common_server/user_auth/abstract/abstract.h>
#include <kernel/common_server/tags/manager.h>
#include <kernel/common_server/geoareas/manager.h>
#include <kernel/common_server/geobase/impl/geobase.h>
#include <kernel/common_server/certs/manager.h>
#include <kernel/common_server/library/scheme/handler.h>
#include <kernel/common_server/obfuscator/manager.h>
#include <kernel/common_server/proposition/manager.h>
#include <kernel/common_server/resources/manager.h>

#include <kernel/common_server/common/url_matcher.h>
#include <kernel/common_server/migrations/manager.h>
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/api/snapshots/controller.h>
#include <kernel/common_server/secret/abstract/manager.h>
#include <kernel/common_server/library/storage/balancing/abstract.h>

class TMessageOnAfterDatabaseConstructed: public IMessage {
private:
    const TMap<TString, TDatabasePtr>& Databases;
public:
    TMessageOnAfterDatabaseConstructed(TMap<TString, TDatabasePtr>& databases)
        : Databases(databases)
    {

    }

    TDatabasePtr GetDatabase(const TString& dbName) const {
        auto it = Databases.find(dbName);
        if (it == Databases.end()) {
            return nullptr;
        }
        return it->second;
    }
};

class TCollectFrontendServerInfo: public TCollectServerInfo {
};

namespace NFrontend {
    class TController: public NController::TController {
    private:
        using TBase = NController::TController;
    public:
        using TBase::TBase;

        virtual ~TController() = default;
    };
}

class TBaseServer
    : public NController::IServer
    , public virtual IBaseServer
    , public NCS::TExternalServicesOperator
    , public IMessageProcessor
    , public NCS::NLogging::ILogsAlertsNotifier
{
private:
    class TRequestHandlersMtpQueue;
    class THandlerThreadsPoolMonitoring;
    TThreadPool MonitoringThreads;
private:
    const ::TBaseServerConfig& Config;

    THolder<NCS::TCertsManger> CertsManager;

    TAtomicSharedPtr<NFrontend::IReadSettings> SafeSettings;
    ISettings::TPtr Settings;

    THolder<TFrontendHttpServer> HttpServer;
    TMap<TString, TAtomicSharedPtr<TRequestHandlersMtpQueue>> RequestHandlers;
    TMap<TString, NRTProc::IVersionedStorage::TPtr> Storages;

    THolder<TNotifiersManager> NotifiersManager;
    THolder<ILocksManager> LocksManager;

    TMap<TString, NCS::IAbstractCipher::TPtr> Ciphers;
    TMap<TString, NCS::IElSignature::TPtr> ElSignatures;

    TMap<TString, IFrontendNotifier::TPtr> Notifications;
    THolder<NCS::ITvmManager> TvmManager;
    THolder<ILocalization> Localization;
    IAuthUsersManager::TPtr AuthUsersManager;
    THolder<TTagDescriptionsManager> TagDescriptionsManager;

    TAtomic IsActive = 0;
    THolder<TRTBackgroundManager> RTBackgroundManager;
    THolder<IRolesManager> RolesManager;
    THolder<IPermissionsManager> PermissionsManager;

    THolder<NCS::NResources::TDBManager> ResourcesManager;

    THolder<IPersonalDataStorage> PersonalDataStorage;
    THolder<IGeoAreasManager> GeoAreas;
    THolder<IGeobaseProvider> Geobase;
    THolder<NCS::IEmulationsManager> EmulationsManager;

    class TPQConstructionContext;
    THolder<TPQConstructionContext> PQConstructionContext;
    class TDBConstructionContext;

    THolder<NCS::NPropositions::TDBManager> PropositionsManager;
    THolder<NCS::TSnapshotsController> SnapshotsController;
    TAtomicSharedPtr<NCS::NObfuscator::IObfuscatorManager> ObfuscatorManager;

    class THandlersActualizationThread;
    TThreadPool HandlersActualizationThreads;
    mutable TRWMutex MutexHandlerInfoSwitch;
    mutable TMap<NCS::TPathHandlerInfo, IRequestProcessorConfig::TPtr> HandlerPrefixesInfo;
    TMap<TString, NCS::NSecret::IManager::TPtr> SecretManagers;
    bool RefreshProcessorsInfo() const;
protected:
    TMap<TString, TDatabasePtr> Databases;
    TMap<TString, NCS::NStorage::TDBMigrationsManager::TPtr> DBMigrationManagers;

    virtual void LoggingNotify(const TString& notifierId, const NCS::NLogging::TBaseLogRecord& r) const override;
public:
    using TController = NFrontend::TController;
    using TConfig = ::TBaseServerConfig;
    using TInfoCollector = ::TCollectFrontendServerInfo;

protected:
    virtual void DoRun() {
    };

    virtual void DoStop(ui32 /*rigidStopLevel*/, const TCgiParameters* /*cgiParams*/ = nullptr) {
    }

public:
    TBaseServer(const TConfig& config);
    ~TBaseServer();
    bool GetIsActive() const {
        return AtomicGet(IsActive);
    }

    virtual bool IsNoPermissionsMode() const override final {
        return Config.IsNoPermissionsMode();
    }

    virtual NCS::NSecret::IManager::TPtr GetSecretManager(const TString& managerId) const override {
        auto it = SecretManagers.find(managerId);
        if (it == SecretManagers.end()) {
            return nullptr;
        }
        return it->second;
    }

    virtual TSet<TString> GetSecretManagerNames() const override {
        return MakeSet(NContainer::Keys(SecretManagers));
    }

    virtual const NCS::NPropositions::TDBManager* GetPropositionsManager() const override;

    virtual NCS::NObfuscator::TObfuscatorManagerContainer GetObfuscatorManager() const override;

    virtual const NCS::IPQConstructionContext& GetPQConstructionContext() const override;

    virtual NFrontend::TConstantsInfoReport BuildInterfaceConstructor(TAtomicSharedPtr<IUserPermissions> permissions) const override;
    virtual TVector<NCS::NScheme::THandlerScheme> BuildHandlersScheme(const TSet<TString>& endpoints) const override;

    virtual TMaybe<NSimpleMeta::TConfig> GetRequestConfig(const TString& apiName, const NNeh::THttpRequest& request) const override;

    virtual const NCS::IEmulationsManager* GetEmulationsManager() const override {
        return EmulationsManager.Get();
    }

    virtual const IGeobaseProvider& GetGeobase() const override {
        return *Geobase;
    }
    const IPersonalDataStorage& GetPersonalDataStorage() const override {
        CHECK_WITH_LOG(PersonalDataStorage);
        return *PersonalDataStorage;
    }

    const IGeoAreasManager* GetGeoAreas() const override {
        return GeoAreas.Get();
    }

    virtual IGeoAreasManager* GetGeoAreas() {
        return GeoAreas.Get();
    }

    virtual const TString& GetServiceName() const override {
        return Config.GetService();
    }

    virtual const TString& GetCType() const override {
        return Config.GetCType();
    }

    virtual TString Name() const override {
        return "BaseFrontend";
    }
    virtual bool Process(IMessage* message) override;

    virtual const ILocalization& GetLocalization() const override {
        CHECK_WITH_LOG(!!Localization);
        return *Localization;
    }

    virtual const ISettings& GetSettings() const override {
        return *Settings;
    }

    virtual bool HasSettings() const override {
        return !!Settings;
    }

    virtual const ILocksManager& GetLocksManager() const override {
        return *LocksManager;
    }

    virtual const TTagDescriptionsManager& GetTagDescriptionsManager() const override {
        return *TagDescriptionsManager;
    }

    virtual const IAuthUsersManager& GetAuthUsersManager() const override {
        return *AuthUsersManager;
    }

    virtual const TNotifiersManager* GetNotifiersManager() const override {
        return NotifiersManager.Get();
    }

    virtual const TS3Client* GetMDSClient() const override {
        return nullptr;
    }

    virtual const NCS::NResources::TDBManager* GetResourcesManager() const override {
        return ResourcesManager.Get();
    }

    virtual TSet<TString> GetElSignatureIds() const override {
        return MakeSet(NContainer::Keys(ElSignatures));
    }

    virtual NCS::IElSignature::TPtr GetElSignature(const TString& id) const override {
        auto it = ElSignatures.find(id);
        if (it == ElSignatures.end()) {
            return nullptr;
        }
        return it->second;
    }

    virtual TSet<TString> GetNotifierNames() const override {
        TSet<TString> result = MakeSet(NContainer::Keys(Notifications));
        if (NotifiersManager) {
            const TSet<TString> dbNotifiers = NotifiersManager->GetObjectNames();
            result.insert(dbNotifiers.begin(), dbNotifiers.end());
        }
        return result;
    }

    virtual TAtomicSharedPtr<IFrontendNotifier> GetNotifier(const TString& name) const override;
    virtual const IPermissionsManager& GetPermissionsManager() const override;

    virtual const TRTBackgroundManager* GetRTBackgroundManager() const override {
        return RTBackgroundManager.Get();
    }

    virtual const NCS::ISnapshotsController& GetSnapshotsController() const override {
        CHECK_WITH_LOG(!!SnapshotsController) << "incorrect snapshots controller usage" << Endl;
        return *SnapshotsController;
    }

    virtual const NCS::IAbstractCipher::TPtr GetCipherPtr(const TString& cipherName) const override {
        auto cipherIt = Ciphers.find(cipherName);
        return cipherIt != Ciphers.end() ? cipherIt->second : nullptr;
    }

    virtual const NCS::IAbstractCipher* GetCipher(const TString& cipherName) const override;

    virtual TSet<TString> GetCipherNames() const override {
        return MakeSet(NContainer::Keys(Ciphers));
    }

    virtual NStorage::IDatabase::TPtr GetDatabase(const TString& name) const override {
        auto it = Databases.find(name);
        if (it == Databases.end()) {
            return nullptr;
        } else {
            return it->second;
        }
    }

    virtual NCS::NStorage::TDBMigrationsManager::TPtr GetDBMigrationsManager(const TString& dbName) const override;

    virtual TVector<TString> GetDatabaseNames() const override {
        TVector<TString> result;
        result.reserve(Databases.size());
        for (auto&& it : Databases) {
            result.emplace_back(it.first);
        }
        return result;
    }

    const THttpStatusManagerConfig& GetHttpStatusManagerConfig() const override {
        return Config.GetHttpStatusManagerConfig();
    }

    const TConfig& GetConfig() const {
        return Config;
    }

    virtual const IRolesManager& GetRolesManager() const override {
        return *RolesManager;
    }

    virtual IUserPermissions::TPtr BuildPermissionFromItems(const TVector<TItemPermissionContainer>& items, const TString& userId) const override;
    virtual TRequestProcessorConfigContainer GetProcessorInfo(const TString& handlerName) const override;
    virtual bool HasProcessorInfo(const TString& handlerName) const override;

    virtual TSet<TString> GetAuthModuleIds() const override {
        return GetConfig().GetAuthModuleIds();
    }

    virtual const IAuthModuleConfig* GetAuthModuleInfo(const TString& authModuleName) const override {
        return GetConfig().GetAuthModuleInfo(authModuleName);
    }

    virtual NRTProc::IVersionedStorage::TPtr GetVersionedStorage(const TString& storageName) const override {
        auto it = Storages.find(storageName);
        if (it != Storages.end()) {
            return it->second;
        } else {
            return nullptr;
        }
    }

    virtual TSet<TString> GetServerProcessors() const override {
        return Config.GetServerProcessors();
    }

    virtual IThreadPool* GetRequestsHandler(const TString& handlerName) const override;

    NCS::ITvmManager* GetTvmManager() const override {
        return TvmManager.Get();
    }

    virtual const NCS::TCertsManger& GetCertsManger() const override;

    virtual void Run() override;
    virtual void Stop(ui32 rigidStopLevel, const TCgiParameters* cgiParams = nullptr) override;
};
