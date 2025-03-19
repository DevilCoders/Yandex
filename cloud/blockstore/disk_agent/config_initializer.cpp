#include "config_initializer.h"

#include "options.h"

#include <cloud/blockstore/libs/diagnostics/config.h>
#include <cloud/blockstore/libs/server/config.h>
#include <cloud/blockstore/libs/spdk/config.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/features_config.h>
#include <cloud/blockstore/libs/storage/disk_agent/config.h>
#include <cloud/blockstore/libs/storage/disk_registry_proxy/config.h>
#include <cloud/blockstore/libs/storage/init/diskagent_actorsystem.h>
#include <cloud/storage/core/libs/kikimr/actorsystem.h>
#include <cloud/storage/core/libs/version/version.h>

#include <library/cpp/protobuf/util/pb_io.h>

#include <util/datetime/base.h>
#include <util/stream/file.h>
#include <util/stream/str.h>

namespace NCloud::NBlockStore::NServer {

namespace {

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

}; // namespace

////////////////////////////////////////////////////////////////////////////////

void TConfigInitializer::ApplyCMSConfigs(NKikimrConfig::TAppConfig cmsConfig)
{
    if (cmsConfig.HasBlobStorageConfig()) {
        KikimrConfig->MutableBlobStorageConfig()
            ->Swap(cmsConfig.MutableBlobStorageConfig());
    }

    if (cmsConfig.HasDomainsConfig()) {
        KikimrConfig->MutableDomainsConfig()
            ->Swap(cmsConfig.MutableDomainsConfig());
    }

    if (cmsConfig.HasNameserviceConfig()) {
        KikimrConfig->MutableNameserviceConfig()
            ->Swap(cmsConfig.MutableNameserviceConfig());
    }

    if (cmsConfig.HasDynamicNameserviceConfig()) {
        KikimrConfig->MutableDynamicNameserviceConfig()
            ->Swap(cmsConfig.MutableDynamicNameserviceConfig());
    }

    ApplyCustomCMSConfigs(cmsConfig);
}

void TConfigInitializer::InitKikimrConfig()
{
    KikimrConfig = std::make_shared<NKikimrConfig::TAppConfig>();

    auto& logConfig = *KikimrConfig->MutableLogConfig();
    if (Options->LogConfig) {
        ParseProtoTextFromFile(Options->LogConfig, logConfig);
    }

    SetupLogConfig(logConfig);

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

    auto& monConfig = *KikimrConfig->MutableMonitoringConfig();
    if (Options->MonitoringConfig) {
        ParseProtoTextFromFile(Options->MonitoringConfig, monConfig);
    }
    SetupMonitoringConfig(monConfig);

    auto& restartsCountConfig = *KikimrConfig->MutableRestartsCountConfig();
    if (Options->RestartsCountFile) {
        restartsCountConfig.SetRestartsCountFile(Options->RestartsCountFile);
    }

    auto& nameServiceConfig = *KikimrConfig->MutableNameserviceConfig();
    if (Options->NameServiceConfig) {
        ParseProtoTextFromFile(
            Options->NameServiceConfig,
            nameServiceConfig
        );
    }

    if (Options->SuppressVersionCheck) {
        nameServiceConfig.SetSuppressVersionCheck(true);
    }

    auto& dynamicNameServiceConfig =
        *KikimrConfig->MutableDynamicNameserviceConfig();
    if (Options->DynamicNameServiceConfig) {
        ParseProtoTextFromFile(
            Options->DynamicNameServiceConfig,
            dynamicNameServiceConfig
        );
    }

    if (Options->AuthConfig) {
        auto& authConfig = *KikimrConfig->MutableAuthConfig();
        ParseProtoTextFromFile(Options->AuthConfig, authConfig);
    }

    auto& bsConfig = *KikimrConfig->MutableBlobStorageConfig();
    bsConfig.MutableServiceSet()->AddAvailabilityDomains(1);
}

void TConfigInitializer::InitDiagnosticsConfig()
{
    NProto::TDiagnosticsConfig diagnosticsConfig;
    if (Options->DiagnosticsConfig) {
        ParseProtoTextFromFile(Options->DiagnosticsConfig, diagnosticsConfig);
    }

    if (Options->MonitoringPort) {
        diagnosticsConfig.SetNbsMonPort(Options->MonitoringPort);
    }

    DiagnosticsConfig = std::make_shared<TDiagnosticsConfig>(diagnosticsConfig);
}

void TConfigInitializer::InitStorageConfig()
{
    NProto::TStorageServiceConfig storageConfig;
    if (Options->StorageConfig) {
        ParseProtoTextFromFile(Options->StorageConfig, storageConfig);
    }

    if (Options->MeteringFile) {
        storageConfig.SetMeteringFilename(Options->MeteringFile);
    }

    if (Options->SchemeShardDir) {
        storageConfig.SetSchemeShardDir(GetFullSchemeShardDir());
    }

    storageConfig.SetServiceVersionInfo(GetFullVersionString());

    StorageConfig = std::make_shared<NStorage::TStorageConfig>(
        storageConfig,
        FeaturesConfig);
}

void TConfigInitializer::InitDiskAgentConfig()
{
    NProto::TDiskAgentConfig diskAgentConfig;
    if (Options->DiskAgentConfig) {
        ParseProtoTextFromFile(Options->DiskAgentConfig, diskAgentConfig);
    }

    SetupDiskAgentConfig(diskAgentConfig);
    ApplySpdkEnvConfig(diskAgentConfig.GetSpdkEnvConfig());

    DiskAgentConfig = std::make_shared<NStorage::TDiskAgentConfig>(
        std::move(diskAgentConfig),
        Rack
    );
}

void TConfigInitializer::InitDiskRegistryProxyConfig()
{
    NProto::TDiskRegistryProxyConfig config;
    if (Options->DiskRegistryProxyConfig) {
        ParseProtoTextFromFile(Options->DiskRegistryProxyConfig, config);
    }

    DiskRegistryProxyConfig = std::make_shared<NStorage::TDiskRegistryProxyConfig>(
        std::move(config));
}

void TConfigInitializer::InitServerConfig()
{
    NProto::TServerAppConfig appConfig;
    if (Options->ServerConfig) {
        ParseProtoTextFromFile(Options->ServerConfig, appConfig);
    }

    auto& serverConfig = *appConfig.MutableServerConfig();

    if (Options->LoadCmsConfigs) {
        serverConfig.SetLoadCmsConfigs(true);
    }

    ServerConfig = std::make_shared<TServerAppConfig>(appConfig);
}

void TConfigInitializer::InitSpdkEnvConfig()
{
    NProto::TSpdkEnvConfig config;
    SpdkEnvConfig = std::make_shared<NSpdk::TSpdkEnvConfig>(config);
}

void TConfigInitializer::InitFeaturesConfig()
{
    NProto::TFeaturesConfig featuresConfig;

    if (Options->FeaturesConfig) {
        ParseProtoTextFromFile(Options->FeaturesConfig, featuresConfig);
    }

    FeaturesConfig =
        std::make_shared<NStorage::TFeaturesConfig>(featuresConfig);
}

NKikimrConfig::TLogConfig TConfigInitializer::GetLogConfig() const
{
    // TODO: move to custom config
    NKikimrConfig::TLogConfig logConfig;
    if (Options->LogConfig) {
        ParseProtoTextFromFile(Options->LogConfig, logConfig);
    }

    SetupLogConfig(logConfig);
    return logConfig;
}

NKikimrConfig::TMonitoringConfig TConfigInitializer::GetMonitoringConfig() const
{
    // TODO: move to custom config
    NKikimrConfig::TMonitoringConfig monConfig;
    if (Options->MonitoringConfig) {
        ParseProtoTextFromFile(Options->MonitoringConfig, monConfig);
    }

    SetupMonitoringConfig(monConfig);
    return monConfig;
}

void TConfigInitializer::SetupMonitoringConfig(NKikimrConfig::TMonitoringConfig& monConfig) const
{
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
}

void TConfigInitializer::SetupLogConfig(NKikimrConfig::TLogConfig& logConfig) const
{
    if (Options->SysLogService) {
        logConfig.SetSysLogService(Options->SysLogService);
    }

    if (Options->VerboseLevel) {
        auto level = GetLogLevel(Options->VerboseLevel);
        Y_ENSURE(level, "unknown log level: " << Options->VerboseLevel.Quote());

        logConfig.SetDefaultLevel(*level);
    }
}

void TConfigInitializer::SetupDiskAgentConfig(NProto::TDiskAgentConfig& config) const
{
    if (!config.GetDedicatedDiskAgent()) {
        config.SetEnabled(false);
    }
}

TString TConfigInitializer::GetFullSchemeShardDir() const
{
    return "/" + Options->Domain + "/" + Options->SchemeShardDir;
}

void TConfigInitializer::ApplyLogConfig(const TString& text)
{
    auto& logConfig = *KikimrConfig->MutableLogConfig();
    ParseProtoTextFromString(text, logConfig);

    SetupLogConfig(logConfig);
}

void TConfigInitializer::ApplyAuthConfig(const TString& text)
{
    ParseProtoTextFromString(text, *KikimrConfig->MutableAuthConfig());
}

void TConfigInitializer::ApplyFeaturesConfig(const TString& text)
{
    NProto::TFeaturesConfig config;
    ParseProtoTextFromString(text, config);

    FeaturesConfig =
        std::make_shared<NStorage::TFeaturesConfig>(config);

    // features config has changed, update storage config
    StorageConfig->SetFeaturesConfig(FeaturesConfig);
}

void TConfigInitializer::ApplySpdkEnvConfig(const NProto::TSpdkEnvConfig& orig)
{
    NProto::TSpdkEnvConfig config;
    config.CopyFrom(orig);
    SpdkEnvConfig = std::make_shared<NSpdk::TSpdkEnvConfig>(std::move(config));
}

void TConfigInitializer::ApplyServerAppConfig(const TString& text)
{
    NProto::TServerAppConfig appConfig;
    ParseProtoTextFromString(text, appConfig);

    ServerConfig = std::make_shared<TServerAppConfig>(appConfig);
}

void TConfigInitializer::ApplyMonitoringConfig(const TString& text)
{
    auto& monConfig = *KikimrConfig->MutableMonitoringConfig();
    ParseProtoTextFromString(text, monConfig);

    SetupMonitoringConfig(monConfig);
}

void TConfigInitializer::ApplyActorSystemConfig(const TString& text)
{
    ParseProtoTextFromString(text, *KikimrConfig->MutableActorSystemConfig());
}

void TConfigInitializer::ApplyDiagnosticsConfig(const TString& text)
{
    NProto::TDiagnosticsConfig diagnosticsConfig;
    ParseProtoTextFromString(text, diagnosticsConfig);

    if (Options->MonitoringPort) {
        diagnosticsConfig.SetNbsMonPort(Options->MonitoringPort);
    }

    DiagnosticsConfig = std::make_shared<TDiagnosticsConfig>(diagnosticsConfig);
}

void TConfigInitializer::ApplyInterconnectConfig(const TString& text)
{
    auto& interconnectConfig = *KikimrConfig->MutableInterconnectConfig();
    ParseProtoTextFromString(text, interconnectConfig);

    interconnectConfig.SetStartTcp(true);
}

void TConfigInitializer::ApplyStorageServiceConfig(const TString& text)
{
    NProto::TStorageServiceConfig storageConfig;
    ParseProtoTextFromString(text, storageConfig);

    if (Options->MeteringFile) {
        storageConfig.SetMeteringFilename(Options->MeteringFile);
    }
    storageConfig.SetServiceVersionInfo(GetFullVersionString());
    StorageConfig = std::make_shared<NStorage::TStorageConfig>(
        storageConfig,
        FeaturesConfig);

    Y_ENSURE(!Options->SchemeShardDir ||
        GetFullSchemeShardDir() == StorageConfig->GetSchemeShardDir());
}

void TConfigInitializer::ApplyDiskAgentConfig(const TString& text)
{
    NProto::TDiskAgentConfig config;
    ParseProtoTextFromString(text, config);

    SetupDiskAgentConfig(config);
    ApplySpdkEnvConfig(config.GetSpdkEnvConfig());

    DiskAgentConfig = std::make_shared<NStorage::TDiskAgentConfig>(
        std::move(config),
        Rack);
}

void TConfigInitializer::ApplyDiskRegistryProxyConfig(const TString& text)
{
    NProto::TDiskRegistryProxyConfig config;
    ParseProtoTextFromString(text, config);

    DiskRegistryProxyConfig = std::make_shared<NStorage::TDiskRegistryProxyConfig>(
        std::move(config));
}

void TConfigInitializer::ApplyCustomCMSConfigs(const NKikimrConfig::TAppConfig& config)
{
    using TSelf = TConfigInitializer;
    using TApplyFn = void (TSelf::*)(const TString&);

    const THashMap<TString, TApplyFn> map {
        { "ActorSystemConfig",       &TSelf::ApplyActorSystemConfig       },
        { "AuthConfig",              &TSelf::ApplyAuthConfig              },
        { "DiagnosticsConfig",       &TSelf::ApplyDiagnosticsConfig       },
        { "DiskAgentConfig",         &TSelf::ApplyDiskAgentConfig         },
        { "DiskRegistryProxyConfig", &TSelf::ApplyDiskRegistryProxyConfig },
        { "InterconnectConfig",      &TSelf::ApplyInterconnectConfig      },
        { "LogConfig",               &TSelf::ApplyLogConfig               },
        { "MonitoringConfig",        &TSelf::ApplyMonitoringConfig        },
        { "ServerAppConfig",         &TSelf::ApplyServerAppConfig         },
        { "FeaturesConfig",          &TSelf::ApplyFeaturesConfig          },
        { "StorageServiceConfig",    &TSelf::ApplyStorageServiceConfig    },
    };

    for (auto& item : config.GetNamedConfigs()) {
        TStringBuf name = item.GetName();
        if (!name.SkipPrefix("Cloud.NBS.")) {
            continue;
        }

        auto it = map.find(name);
        if (it != map.end()) {
            std::invoke(it->second, this, item.GetConfig());
        }
    }
}

}   // namespace NCloud::NBlockStore::NServer
