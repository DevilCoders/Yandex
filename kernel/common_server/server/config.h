#pragma once

#include <kernel/common_server/api/security/security.h>
#include <kernel/common_server/api/localization/config.h>
#include <kernel/common_server/settings/abstract/config.h>
#include <kernel/common_server/auth/common/auth.h>
#include <kernel/common_server/common/processor.h>
#include <kernel/common_server/notifications/abstract/abstract.h>
#include <kernel/common_server/rt_background/manager.h>
#include <kernel/common_server/personal_data/storage.h>
#include <kernel/common_server/library/async_proxy/async_delivery.h>
#include <kernel/common_server/certs/config.h>
#include <kernel/common_server/library/searchserver/simple/http_status_config.h>
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <kernel/common_server/emulation/abstract/manager.h>
#include <kernel/common_server/resources/config.h>
#include <kernel/common_server/geobase/config/config.h>

#include <kernel/daemon/config/config_constructor.h>
#include <kernel/daemon/config/daemon_config.h>
#include <kernel/daemon/module/module.h>

#include <library/cpp/yconf/conf.h>
#include <library/cpp/logger/backend_creator.h>

#include <util/stream/output.h>
#include <kernel/common_server/notifications/manager.h>
#include <kernel/common_server/locks/abstract.h>
#include <kernel/common_server/user_auth/abstract/abstract.h>
#include <kernel/common_server/tags/config.h>
#include <kernel/common_server/user_role/abstract/abstract.h>
#include <kernel/common_server/auth/common/tvm_config.h>
#include <kernel/common_server/proposition/config.h>
#include <kernel/common_server/obfuscator/config.h>

#include <kernel/common_server/geoareas/manager.h>

#include <kernel/common_server/util/algorithm/container.h>
#include <kernel/common_server/signature/abstract/abstract.h>
#include <kernel/common_server/library/persistent_queue/abstract/config.h>
#include <kernel/common_server/api/snapshots/config.h>
#include <kernel/common_server/secret/abstract/config.h>
#include <kernel/common_server/ciphers/config.h>
#include <kernel/common_server/migrations/config.h>

class TRequestsHandlerConfig {
private:
    CSA_READONLY(ui32, ThreadsCount, 8);
    CSA_READONLY(ui32, QueueSizeLimit, 512);
public:
    void Init(const TYandexConfig::Section* section) {
        ThreadsCount = section->GetDirectives().Value("ThreadsCount", ThreadsCount);
        QueueSizeLimit = section->GetDirectives().Value("QueueSizeLimit", QueueSizeLimit);
    }

    void ToString(IOutputStream& os) const {
        os << "ThreadsCount: " << ThreadsCount << Endl;
        os << "QueueSizeLimit: " << QueueSizeLimit << Endl;
    }
};

class TRequestProcessingConfig {
private:
    CSA_READONLY_DEF(TString, DefaultType);
    CSA_READONLY_DEF(TVector<TString>, HeadersToForward)

public:
    void Init(const TYandexConfig::Section* section);
    void ToString(IOutputStream& os) const;
};

class TEventLogConfig {
public:
    THolder<TLogBackend> CreateLogBackend() const;
    void Init(const TYandexConfig::Section* section);
    void ToString(IOutputStream& os) const;

    CSA_READONLY(NCS::NLogging::ELogRecordFormat, Format, NCS::NLogging::ELogRecordFormat::TSKV);
    CSA_READONLY(ELogPriority, Priority, LOG_DEF_PRIORITY);
    CSA_READONLY_FLAG(BackgroundUsage, false);
    CSA_READONLY(bool, SkipOnQueueLimitExceeded, true);
    CSA_READONLY(ui32, QueueSizeLimit, 512);
    
private:
    THolder<ILogBackendCreator> BackendCreator;
};

class TBaseServerConfig: public IServerConfig {
private:
    CSA_READONLY_DEF(TMonitoringConfig, MonitoringConfig);
    CSA_READONLY_DEF(TRolesManagerConfig, RolesManagerConfig);
    CSA_READONLY_DEF(TPermissionsManagerConfig, PermissionsManagerConfig);
    CSA_READONLY_DEF(NCS::TElSignaturesConfig, ElSignaturesConfig);
    CSA_READONLY_DEF(NCS::TGeobaseConfig, GeobaseConfig);
    CSA_READONLY_DEF(NCS::NResources::TDBManagerConfig, ResourcesManagerConfig);
    CSA_READONLY_DEF(NCS::NPropositions::TDBManagerConfig, PropositionsManagerConfig);
    CSA_READONLY_DEF(NCS::NObfuscator::TDBManagerConfig, ObfuscatorManagerConfig);
    CSA_READONLY_DEF(NCS::TCertsMangerConfig, CertsManagerConfig);
    NCS::TSnapshotsControllerConfig SnapshotsControllerConfig;
    NCS::TEmulationsManagerConfig EmulationsManagerConfig;
public:
    class TSensorApiConfig {
    public:
        void Init(const TYandexConfig::Section* section) {
            CHECK_WITH_LOG(section);
            const auto& d = section->GetDirectives();
            SensorApiName = d.Value("Name", SensorApiName);
            HistoryApiName = d.Value("HistoryApiName", HistoryApiName);
            HistoryApiTimeout = d.Value("HistoryApiTimeout", HistoryApiTimeout);
        }

        void ToString(IOutputStream& os) const {
            os << "Name: " << SensorApiName  << Endl;
            os << "HistoryApiName: " << HistoryApiName << Endl;
            os << "HistoryApiTimeout: " << HistoryApiTimeout << Endl;
        }

    private:
        CSA_READONLY_DEF(TString, SensorApiName);
        CSA_READONLY_DEF(TString, HistoryApiName);
        CSA_READONLY(TDuration, HistoryApiTimeout, TDuration::Seconds(2));
    };
public:
    const TDaemonConfig DaemonConfig;

protected:
    TMap<TString, TRequestsHandlerConfig> RequestHandlers;
    TDaemonConfig::THttpOptions HttpServerOptions;
    THttpStatusManagerConfig HttpStatusManagerConfig;
    TRequestProcessingConfig RequestProcessingConfig;
    TLocksManagerConfigContainer LocksManagerConfigContainer;
    TMap<TString, IFrontendNotifierConfig::TPtr> Notifications;
    THolder<TNotifiersManagerConfig> NotifiersManagerConfig;
    THolder<TSecretsManagerConfig> SecretsManagerConfig;
    TMap<TString, IRequestProcessorConfig::TPtr> ProcessorsInfo;
    TMap<TString, IAuthModuleConfig::TPtr> AuthModuleConfigs;
    TMap<TString, NRTProc::TStorageOptions> DatabaseConfigs;
    TMap<TString, NSimpleMeta::TConfig> RequestPolicy;
    TMap<TString, TTvmConfig> TvmConfigs;
    THolder<TRTBackgroundManagerConfig> RTBackgroundManagerConfig;
    TTagDescriptionsManagerConfig TagDescriptionsConfig;
    THolder<NCS::NLocalization::TConfig> Localization;
    TAuthUsersManagerConfig AuthUsersManagerConfig;
    NFrontend::TSettingsConfigContainer Settings;
    TMap<TString, NStorage::TDatabaseConfig> Databases;
    TPluginConfigs<IDaemonModuleConfig> ModulesConfig;
    TSet<TString> UsingModules;
    THolder<TAsyncDeliveryResources::TConfig> ADGlobalConfig;
    TMap<TString, NCS::ICipherConfig::TPtr> Ciphers;
    TMap<TString, NExternalAPI::TSenderConfig> AbstractExternalAPI;
    TMap<TString, NCS::TPQClientConfigContainer> ExternalQueueConfigs;
    TGeoAreasManagerConfig GeoAreasConfig;
    using TDBMigrationManagersConfig = TMap<TString, NCS::NStorage::TDBMigrationsConfig>;
    CSA_READONLY_DEF(TDBMigrationManagersConfig, DBMigrationManagerConfigs);
    using TSecretManagerConfigs = TMap<TString, NCS::NSecret::TSecretsManagerConfig>;
    CSA_READONLY_DEF(TSecretManagerConfigs, SecretManagerConfigs);

    TEventLogConfig EventLog;
    CSA_FLAG(TBaseServerConfig, NoPermissionsMode, false);

    CSA_READONLY_DEF(TPersonalDataStorageConfig, PersonalDataStorageConfig);
private:
    virtual void OnAfterCreate(const TServerConfigConstructorParams& params) final override;
protected:
    const IAbstractModuleConfig* GetModuleConfigImpl(const TString& name) const override {
        return ModulesConfig.Get<IAbstractModuleConfig>(name);
    }
    virtual void Init(const TYandexConfig::Section* section);
    virtual void DoToString(IOutputStream& /*os*/) const {
    }

public:
    TBaseServerConfig(const TServerConfigConstructorParams& params);

    const NCS::TSnapshotsControllerConfig& GetSnapshotsControllerConfig() const {
        return SnapshotsControllerConfig;
    }

    const NCS::TEmulationsManagerConfig& GetEmulationsManagerConfig() const {
        return EmulationsManagerConfig;
    }

    const TTagDescriptionsManagerConfig& GetTagDescriptionsConfig() const {
        return TagDescriptionsConfig;
    }

    const TGeoAreasManagerConfig& GetGeoAreasConfig() const {
        return GeoAreasConfig;
    }

    const TAsyncDeliveryResources::TConfig* GetADGlobalConfig() const {
        if (!ADGlobalConfig || !ADGlobalConfig->GetEnabled()) {
            return nullptr;
        }
        return ADGlobalConfig.Get();
    }

    TSet<TString> GetServerProcessors() const {
        return MakeSet(NContainer::Keys(ProcessorsInfo));
    }

    const TMap<TString, IRequestProcessorConfig::TPtr>& GetServerProcessorsObjects() const {
        return ProcessorsInfo;
    }

    const TMap<TString, IFrontendNotifierConfig::TPtr>& GetNotifications() const {
        return Notifications;
    }

    const TMap<TString, NCS::TPQClientConfigContainer>& GetExternalQueueConfigs() const {
        return ExternalQueueConfigs;
    }

    const TMap<TString, NExternalAPI::TSenderConfig>& GetAbstractExternalApiConfigs() const {
        return AbstractExternalAPI;
    }

    const TLocksManagerConfigContainer& GetLocksManagerConfigContainer() const {
        return LocksManagerConfigContainer;
    }

    const TAuthUsersManagerConfig& GetAuthUsersManagerConfig() const {
        return AuthUsersManagerConfig;
    }

    const NCS::NLocalization::TConfig* GetLocalizationConfig() const {
        return Localization.Get();
    }

    NFrontend::ISettingsConfig::TPtr GetSettingsConfig() const {
        return Settings.GetPtr();
    }

    const TMap<TString, NRTProc::TStorageOptions>& GetDatabases() const {
        return DatabaseConfigs;
    }

    const TMap<TString, TTvmConfig>& GetTvmConfigs() const {
        return TvmConfigs;
    }

    const TMap<TString, NCS::ICipherConfig::TPtr>& GetCiphers() const {
        return Ciphers;
    }

    TRTBackgroundManagerConfig* GetRTBackgroundManagerConfig() const {
        return RTBackgroundManagerConfig.Get();
    }

    TNotifiersManagerConfig* GetNotifiersManagerConfig() const {
        return NotifiersManagerConfig.Get();
    }

    TSecretsManagerConfig* GetSecretsManagerConfig() const {
        return SecretsManagerConfig.Get();
    }

    const TMap<TString, NStorage::TDatabaseConfig>& GetExternalDatabases() const;

    const IAuthModuleConfig* GetAuthModuleInfo(const TString& authModuleName) const {
        auto it = AuthModuleConfigs.find(authModuleName);
        if (it == AuthModuleConfigs.end()) {
            return nullptr;
        }
        return it->second.Get();
    }

    TSet<TString> GetAuthModuleIds() const {
        return MakeSet(NContainer::Keys(AuthModuleConfigs));
    }

    const TRequestProcessingConfig& GetRequestProcessingConfig() const {
        return RequestProcessingConfig;
    }

    const THttpStatusManagerConfig& GetHttpStatusManagerConfig() const {
        return HttpStatusManagerConfig;
    }

    const THttpServerOptions& GetHttpServerOptions() const {
        return HttpServerOptions;
    }

    virtual TSet<TString> GetModulesSet() const override {
        return UsingModules;
    }

    const TMap<TString, TRequestsHandlerConfig>& GetHandlers() const {
        return RequestHandlers;
    }

    virtual const TDaemonConfig& GetDaemonConfig() const override {
        return DaemonConfig;
    }

    TString ToString() const {
        TStringStream ss;
        ToString(ss);
        return ss.Str();
    }

    void ToString(IOutputStream& os) const;

    const TEventLogConfig& GetEventLog() const {
        return EventLog;
    }
};
