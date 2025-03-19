#pragma once

#include "private.h"

#include <cloud/blockstore/libs/client/config.h>
#include <cloud/blockstore/libs/client/throttling.h>
#include <cloud/blockstore/libs/common/public.h>
#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/discovery/config.h>
#include <cloud/blockstore/libs/discovery/public.h>
#include <cloud/blockstore/libs/endpoints/public.h>
#include <cloud/blockstore/libs/kikimr/public.h>
#include <cloud/blockstore/libs/logbroker/public.h>
#include <cloud/blockstore/libs/notify/public.h>
#include <cloud/blockstore/libs/server/public.h>
#include <cloud/blockstore/libs/service/public.h>
#include <cloud/blockstore/libs/spdk/public.h>
#include <cloud/blockstore/libs/storage_local/public.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/features_config.h>
#include <cloud/blockstore/libs/storage/disk_agent/config.h>
#include <cloud/blockstore/libs/storage/disk_registry_proxy/config.h>
#include <cloud/blockstore/libs/ydbstats/config.h>
#include <cloud/storage/core/libs/coroutine/public.h>

#include <ydb/core/protos/config.pb.h>

#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/logger/log.h>

namespace google::protobuf {

////////////////////////////////////////////////////////////////////////////////

class Message;

}   // namespace google::protobuf

namespace NCloud::NBlockStore::NServer {

////////////////////////////////////////////////////////////////////////////////

void ParseProtoTextFromString(
    const TString& text,
    google::protobuf::Message& dst);

void ParseProtoTextFromFile(
    const TString& fileName,
    google::protobuf::Message& dst);

////////////////////////////////////////////////////////////////////////////////

struct TConfigInitializer
{
    const TOptionsPtr Options;

    NKikimrConfig::TAppConfigPtr KikimrConfig;
    NYdbStats::TYdbStatsConfigPtr StatsConfig;
    NDiscovery::TDiscoveryConfigPtr DiscoveryConfig;
    TServerAppConfigPtr ServerConfig;
    NClient::TClientAppConfigPtr EndpointConfig;
    NStorage::TStorageConfigPtr StorageConfig;
    NStorage::TDiskAgentConfigPtr DiskAgentConfig;
    TLocalStorageConfigPtr LocalStorageConfig;
    NStorage::TDiskRegistryProxyConfigPtr DiskRegistryProxyConfig;
    TDiagnosticsConfigPtr DiagnosticsConfig;
    NSpdk::TSpdkEnvConfigPtr SpdkEnvConfig;
    NStorage::TFeaturesConfigPtr FeaturesConfig;
    NLogbroker::TLogbrokerConfigPtr LogbrokerConfig;
    NNotify::TNotifyConfigPtr NotifyConfig;
    NClient::THostPerformanceProfile HostPerformanceProfile;

    TString Rack;
    TLog Log;

    explicit TConfigInitializer(TOptionsPtr options)
        : Options(std::move(options))
    {}

    void ApplyCMSConfigs(NKikimrConfig::TAppConfig cmsConfig);
    void ApplyCustomCMSConfigs(const NKikimrConfig::TAppConfig& config);

    void InitDiagnosticsConfig();
    void InitDiscoveryConfig();
    void InitDiskAgentConfig();
    void InitDiskRegistryProxyConfig();
    void InitEndpointConfig();
    void InitFeaturesConfig();
    void InitHostPerformanceProfile();
    void InitKikimrConfig();
    void InitLocalStorageConfig();
    void InitLogbrokerConfig();
    void InitNotifyConfig();
    void InitServerConfig();
    void InitSpdkEnvConfig();
    void InitStatsUploadConfig();
    void InitStorageConfig();

    NKikimrConfig::TLogConfig GetLogConfig() const;
    NKikimrConfig::TMonitoringConfig GetMonitoringConfig() const;

private:
    TString GetFullSchemeShardDir() const;
    std::optional<NJson::TJsonValue> ReadJsonFile(const TString& filename);

    void SetupDiscoveryPorts(NProto::TDiscoveryServiceConfig& discoveryConfig) const;
    void SetupServerPorts(NProto::TServerConfig& config) const;
    void SetupStorageConfig(NProto::TStorageServiceConfig& config) const;
    void SetupMonitoringConfig(NKikimrConfig::TMonitoringConfig& monConfig) const;
    void SetupLogLevel(NKikimrConfig::TLogConfig& logConfig) const;
    void SetupGrpcThreadsLimit() const;

    void ApplyActorSystemConfig(const TString& text);
    void ApplyAuthConfig(const TString& text);
    void ApplyDiagnosticsConfig(const TString& text);
    void ApplyDiscoveryServiceConfig(const TString& text);
    void ApplyDiskAgentConfig(const TString& text);
    void ApplyDiskRegistryProxyConfig(const TString& text);
    void ApplyFeaturesConfig(const TString& text);
    void ApplyInterconnectConfig(const TString& text);
    void ApplyLocalStorageConfig(const TString& text);
    void ApplyLogbrokerConfig(const TString& text);
    void ApplyLogConfig(const TString& text);
    void ApplyMonitoringConfig(const TString& text);
    void ApplyNotifyConfig(const TString& text);
    void ApplyServerAppConfig(const TString& text);
    void ApplySpdkEnvConfig(const TString& text);
    void ApplyStorageServiceConfig(const TString& text);
    void ApplyYdbStatsConfig(const TString& text);
};

}   // namespace NCloud::NBlockStore::NServer
