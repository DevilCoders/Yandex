#pragma once

#include "private.h"

#include <util/generic/string.h>

namespace NCloud::NBlockStore::NServer {

////////////////////////////////////////////////////////////////////////////////

struct TOptions
{
    TString LogConfig;
    TString SysConfig;
    TString DomainsConfig;
    TString NameServiceConfig;
    TString DynamicNameServiceConfig;

    TString InterconnectConfig;
    ui32 InterconnectPort = 0;

    TString MonitoringConfig;
    TString MonitoringAddress;
    ui32 MonitoringPort = 0;
    ui32 MonitoringThreads = 0;

    TString Domain;
    TString SchemeShardDir;
    TString NodeBrokerAddress;
    ui32 NodeBrokerPort = 0;
    bool LoadCmsConfigs = false;
    TString LocationFile;

    TString ServerConfig;
    ui32 ServerPort = 0;
    ui32 DataServerPort = 0;
    ui32 SecureServerPort = 0;

    bool TemporaryServer = false;

    TString EndpointConfig;

    TString StorageConfig;
    TString DiskAgentConfig;
    TString LocalStorageConfig;
    TString DiskRegistryProxyConfig;
    TString AuthConfig;
    TString DiagnosticsConfig;
    TString StatsUploadConfig;
    TString DiscoveryConfig;
    TString FeaturesConfig;
    TString LogbrokerConfig;
    TString NotifyConfig;

    TString RestartsCountFile;
    TString MeteringFile;
    TString ProfileFile;

    TString VerboseLevel;
    bool EnableGrpcTracing = false;
    bool MemLock = false;

    enum class EServiceKind {
        Null   /* "null"   */ ,
        Local  /* "local"  */ ,
        Kikimr /* "kikimr" */ ,
    };

    EServiceKind ServiceKind = EServiceKind::Null;

    bool SuppressVersionCheck = false;

    void Parse(int argc, char** argv);
};

}   // namespace NCloud::NBlockStore::NServer
