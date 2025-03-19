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

    TString StorageConfig;
    TString DiskAgentConfig;
    TString DiskRegistryProxyConfig;
    TString AuthConfig;
    TString DiagnosticsConfig;
    TString FeaturesConfig;

    TString RestartsCountFile;
    TString MeteringFile;
    TString ProfileFile;

    TString SysLogService;

    TString VerboseLevel;
    bool MemLock = false;

    bool SuppressVersionCheck = false;

    TString NodeType;

    void Parse(int argc, char** argv);
};

}   // namespace NCloud::NBlockStore::NServer
