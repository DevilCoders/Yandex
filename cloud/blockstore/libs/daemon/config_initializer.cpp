#include "config_initializer.h"

#include "options.h"

#include <cloud/blockstore/libs/client/client.h>
#include <cloud/blockstore/libs/client/config.h>
#include <cloud/blockstore/libs/diagnostics/config.h>
#include <cloud/blockstore/libs/discovery/config.h>
#include <cloud/blockstore/libs/logbroker/config.h>
#include <cloud/blockstore/libs/notify/config.h>
#include <cloud/blockstore/libs/server/config.h>
#include <cloud/blockstore/libs/spdk/config.h>
#include <cloud/blockstore/libs/storage_local/config.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/features_config.h>
#include <cloud/blockstore/libs/storage/disk_agent/config.h>
#include <cloud/blockstore/libs/storage/disk_registry_proxy/config.h>
#include <cloud/blockstore/libs/storage/init/actorsystem.h>
#include <cloud/blockstore/libs/ydbstats/config.h>
#include <cloud/storage/core/libs/grpc/executor.h>
#include <cloud/storage/core/libs/grpc/threadpool.h>
#include <cloud/storage/core/libs/kikimr/actorsystem.h>
#include <cloud/storage/core/libs/version/version.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/datetime/base.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/string/cast.h>

namespace NCloud::NBlockStore::NServer {

using namespace NCloud::NBlockStore::NDiscovery;

namespace {

////////////////////////////////////////////////////////////////////////////////

void ParseProtoTextFromStringUnsafe(
    const TString& text,
    google::protobuf::Message& dst)
{
    TStringInput in(text);
    ParseFromTextFormat(in, dst, EParseFromTextFormatOption::AllowUnknownField);
}

void ParseProtoTextFromFileUnsafe(
    const TString& fileName,
    google::protobuf::Message& dst)
{
    TFileInput in(fileName);
    ParseFromTextFormat(in, dst, EParseFromTextFormatOption::AllowUnknownField);
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void ParseProtoTextFromString(
    const TString& text,
    google::protobuf::Message& dst)
{
    TStringInput in(text);
    ParseFromTextFormat(in, dst);
}

void ParseProtoTextFromFile(
    const TString& fileName,
    google::protobuf::Message& dst)
{
    TFileInput in(fileName);
    ParseFromTextFormat(in, dst);
}

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

    *KikimrConfig->MutableLogConfig() = GetLogConfig();

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

void TConfigInitializer::InitStatsUploadConfig()
{
    NProto::TYdbStatsConfig statsConfig;
    if (Options->StatsUploadConfig) {
        ParseProtoTextFromFile(Options->StatsUploadConfig, statsConfig);
    }

    StatsConfig = std::make_shared<NYdbStats::TYdbStatsConfig>(statsConfig);
}

void TConfigInitializer::InitDiscoveryConfig()
{
    NProto::TDiscoveryServiceConfig discoveryConfig;
    if (Options->DiscoveryConfig) {
        ParseProtoTextFromFile(Options->DiscoveryConfig, discoveryConfig);
    }

    SetupDiscoveryPorts(discoveryConfig);

    DiscoveryConfig = std::make_shared<TDiscoveryConfig>(discoveryConfig);
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

    SetupStorageConfig(storageConfig);

    if (Options->SchemeShardDir) {
        storageConfig.SetSchemeShardDir(GetFullSchemeShardDir());
    }

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

    if (Options->TemporaryServer || diskAgentConfig.GetDedicatedDiskAgent()) {
        diskAgentConfig.SetEnabled(false);
    }

    DiskAgentConfig = std::make_shared<NStorage::TDiskAgentConfig>(
        std::move(diskAgentConfig),
        Rack
    );
}

void TConfigInitializer::InitLocalStorageConfig()
{
    NProto::TLocalStorageConfig config;
    if (Options->LocalStorageConfig) {
        ParseProtoTextFromFile(Options->LocalStorageConfig, config);
    }

    LocalStorageConfig = std::make_shared<TLocalStorageConfig>(std::move(config));
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
    SetupServerPorts(serverConfig);

    if (Options->LoadCmsConfigs) {
        serverConfig.SetLoadCmsConfigs(true);
    }

    ServerConfig = std::make_shared<TServerAppConfig>(appConfig);
    SetupGrpcThreadsLimit();
}

void TConfigInitializer::InitEndpointConfig()
{
    NProto::TClientAppConfig appConfig;
    if (Options->EndpointConfig) {
        ParseProtoTextFromFile(Options->EndpointConfig, appConfig);
    }

    EndpointConfig = std::make_shared<NClient::TClientAppConfig>(appConfig);
}

std::optional<NJson::TJsonValue> TConfigInitializer::ReadJsonFile(
    const TString& filename)
{
    if (filename.empty()) {
        return {};
    }

    try {
        TFileInput in(filename);
        return NJson::ReadJsonTree(&in, true);
    } catch (...) {
        STORAGE_ERROR("Failed to read file: " << filename.Quote()
            << " with error: " << CurrentExceptionMessage().c_str());
        return {};
    }
}

void TConfigInitializer::InitHostPerformanceProfile()
{
    const auto& tc = EndpointConfig->GetClientConfig().GetThrottlingConfig();
    ui32 networkThroughput = tc.GetDefaultNetworkMbitThroughput();
    ui32 hostCpuCount = tc.GetDefaultHostCpuCount();

    if (auto json = ReadJsonFile(tc.GetInfraThrottlingConfigPath())) {
        try {
            if (auto* value = json->GetValueByPath("interfaces.[0].eth0.speed")) {
                networkThroughput = FromString<ui64>(value->GetStringSafe());
            }
        } catch (...) {
            STORAGE_ERROR("Failed to read NetworkMbitThroughput. Error: "
                << CurrentExceptionMessage().c_str());
        }

        try {
            if (auto* value = json->GetValueByPath("compute_cores_num")) {
                hostCpuCount = value->GetUIntegerSafe();
            }
        } catch (...) {
            STORAGE_ERROR("Failed to read HostCpuCount. Error: "
                << CurrentExceptionMessage().c_str());
        }
    }

    HostPerformanceProfile = {
        .CpuCount = hostCpuCount,
        .NetworkMbitThroughput = networkThroughput,
    };
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

void TConfigInitializer::InitLogbrokerConfig()
{
    NProto::TLogbrokerConfig config;

    if (Options->LogbrokerConfig) {
        ParseProtoTextFromFile(Options->LogbrokerConfig, config);
    }

    if (!config.GetCaCertFilename()) {
        config.SetCaCertFilename(ServerConfig->GetRootCertsFile());
    }

    LogbrokerConfig = std::make_shared<NLogbroker::TLogbrokerConfig>(config);
}

void TConfigInitializer::InitNotifyConfig()
{
    NProto::TNotifyConfig config;

    if (Options->NotifyConfig) {
        ParseProtoTextFromFile(Options->NotifyConfig, config);

        if (!config.GetCaCertFilename()) {
            config.SetCaCertFilename(ServerConfig->GetRootCertsFile());
        }
    }

    NotifyConfig = std::make_shared<NNotify::TNotifyConfig>(config);
}

NKikimrConfig::TLogConfig TConfigInitializer::GetLogConfig() const
{
    // TODO: move to custom config
    NKikimrConfig::TLogConfig logConfig;
    if (Options->LogConfig) {
        ParseProtoTextFromFileUnsafe(Options->LogConfig, logConfig);
    }

    SetupLogLevel(logConfig);
    logConfig.SetIgnoreUnknownComponents(true);
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

void TConfigInitializer::SetupDiscoveryPorts(NProto::TDiscoveryServiceConfig& discoveryConfig) const {
    Y_VERIFY(ServerConfig);

    if (!discoveryConfig.GetConductorInstancePort()) {
        discoveryConfig.SetConductorInstancePort(ServerConfig->GetPort());
    }

    if (!discoveryConfig.GetConductorSecureInstancePort()) {
        discoveryConfig.SetConductorSecureInstancePort(ServerConfig->GetSecurePort());
    }
}

void TConfigInitializer::SetupServerPorts(NProto::TServerConfig& config) const
{
    if (Options->ServerPort) {
        config.SetPort(Options->ServerPort);
    }
    if (Options->DataServerPort) {
        config.SetDataPort(Options->DataServerPort);
    }
    if (Options->SecureServerPort) {
        config.SetSecurePort(Options->SecureServerPort);
    }
}

void TConfigInitializer::SetupStorageConfig(NProto::TStorageServiceConfig& config) const
{
    if (Options->MeteringFile) {
        config.SetMeteringFilename(Options->MeteringFile);
    }

    if (Options->TemporaryServer) {
        config.SetRemoteMountOnly(true);
        config.SetInactiveClientsTimeout(Max<ui32>());
        config.SetRejectMountOnAddClientTimeout(true);
    }

    config.SetServiceVersionInfo(GetFullVersionString());
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

void TConfigInitializer::SetupLogLevel(NKikimrConfig::TLogConfig& logConfig) const
{
    if (Options->VerboseLevel) {
        auto level = GetLogLevel(Options->VerboseLevel);
        Y_ENSURE(level, "unknown log level: " << Options->VerboseLevel.Quote());

        logConfig.SetDefaultLevel(*level);
    }
}

void TConfigInitializer::SetupGrpcThreadsLimit() const
{
    ui32 maxThreads = ServerConfig->GetGrpcThreadsLimit();
    SetExecutorThreadsLimit(maxThreads);
    SetDefaultThreadPoolLimit(maxThreads);
}

TString TConfigInitializer::GetFullSchemeShardDir() const
{
    return "/" + Options->Domain + "/" + Options->SchemeShardDir;
}

void TConfigInitializer::ApplyLogConfig(const TString& text)
{
    auto& logConfig = *KikimrConfig->MutableLogConfig();
    ParseProtoTextFromStringUnsafe(text, logConfig);

    SetupLogLevel(logConfig);
    logConfig.SetIgnoreUnknownComponents(true);
}

void TConfigInitializer::ApplyAuthConfig(const TString& text)
{
    ParseProtoTextFromString(text, *KikimrConfig->MutableAuthConfig());
}

void TConfigInitializer::ApplyYdbStatsConfig(const TString& text)
{
    NProto::TYdbStatsConfig statsConfig;
    ParseProtoTextFromString(text, statsConfig);

    StatsConfig = std::make_shared<NYdbStats::TYdbStatsConfig>(statsConfig);
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

void TConfigInitializer::ApplyLogbrokerConfig(const TString& text)
{
    NProto::TLogbrokerConfig config;
    ParseProtoTextFromString(text, config);

    if (!config.GetCaCertFilename()) {
        config.SetCaCertFilename(ServerConfig->GetRootCertsFile());
    }

    LogbrokerConfig = std::make_shared<NLogbroker::TLogbrokerConfig>(config);
}

void TConfigInitializer::ApplyNotifyConfig(const TString& text)
{
    NProto::TNotifyConfig config;
    ParseProtoTextFromString(text, config);

    if (!config.GetCaCertFilename()) {
        config.SetCaCertFilename(ServerConfig->GetRootCertsFile());
    }

    NotifyConfig = std::make_shared<NNotify::TNotifyConfig>(config);
}

void TConfigInitializer::ApplySpdkEnvConfig(const TString& text)
{
    NProto::TSpdkEnvConfig config;
    ParseProtoTextFromString(text, config);
    SpdkEnvConfig = std::make_shared<NSpdk::TSpdkEnvConfig>(config);
}

void TConfigInitializer::ApplyServerAppConfig(const TString& text)
{
    NProto::TServerAppConfig appConfig;
    ParseProtoTextFromString(text, appConfig);

    auto& serverConfig = *appConfig.MutableServerConfig();
    SetupServerPorts(serverConfig);

    ServerConfig = std::make_shared<TServerAppConfig>(appConfig);
    SetupGrpcThreadsLimit();
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

    SetupStorageConfig(storageConfig);

    StorageConfig = std::make_shared<NStorage::TStorageConfig>(
        storageConfig,
        FeaturesConfig);

    Y_ENSURE(!Options->SchemeShardDir ||
        GetFullSchemeShardDir() == StorageConfig->GetSchemeShardDir());
}

void TConfigInitializer::ApplyDiscoveryServiceConfig(const TString& text)
{
    NProto::TDiscoveryServiceConfig discoveryConfig;
    ParseProtoTextFromString(text, discoveryConfig);

    SetupDiscoveryPorts(discoveryConfig);

    DiscoveryConfig = std::make_shared<TDiscoveryConfig>(discoveryConfig);
}

void TConfigInitializer::ApplyDiskAgentConfig(const TString& text)
{
    NProto::TDiskAgentConfig config;
    ParseProtoTextFromString(text, config);

    if (Options->TemporaryServer || config.GetDedicatedDiskAgent()) {
        config.SetEnabled(false);
    }

    DiskAgentConfig = std::make_shared<NStorage::TDiskAgentConfig>(
        std::move(config),
        Rack);
}

void TConfigInitializer::ApplyLocalStorageConfig(const TString& text)
{
    NProto::TLocalStorageConfig config;
    ParseProtoTextFromString(text, config);

    LocalStorageConfig = std::make_shared<TLocalStorageConfig>(std::move(config));
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
        { "DiscoveryServiceConfig",  &TSelf::ApplyDiscoveryServiceConfig  },
        { "DiskAgentConfig",         &TSelf::ApplyDiskAgentConfig         },
        { "DiskRegistryProxyConfig", &TSelf::ApplyDiskRegistryProxyConfig },
        { "FeaturesConfig",          &TSelf::ApplyFeaturesConfig          },
        { "InterconnectConfig",      &TSelf::ApplyInterconnectConfig      },
        { "LocalStorageConfig",      &TSelf::ApplyLocalStorageConfig      },
        { "LogbrokerConfig",         &TSelf::ApplyLogbrokerConfig         },
        { "LogConfig",               &TSelf::ApplyLogConfig               },
        { "MonitoringConfig",        &TSelf::ApplyMonitoringConfig        },
        { "NotifyConfig",            &TSelf::ApplyNotifyConfig            },
        { "ServerAppConfig",         &TSelf::ApplyServerAppConfig         },
        { "SpdkEnvConfig",           &TSelf::ApplySpdkEnvConfig           },
        { "StorageServiceConfig",    &TSelf::ApplyStorageServiceConfig    },
        { "YdbStatsConfig",          &TSelf::ApplyYdbStatsConfig          },
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
