#pragma once

#include <library/cpp/getopt/small/last_getopt.h>

#include <util/generic/string.h>

namespace NCloud::NFileStore::NDaemon
{

////////////////////////////////////////////////////////////////////////////////

enum class EServiceKind
{
    Null   /* "null"   */ ,
    Local  /* "local"  */ ,
    Kikimr /* "kikimr" */ ,
};

////////////////////////////////////////////////////////////////////////////////

struct TBootstrapOptions
{
    TString AppConfig;
    TString VerboseLevel;
    bool NoMemLock = false;

    TString ServerAddress;
    ui32 ServerPort = 0;

    TString MonitoringConfig;
    TString MonitoringAddress;
    ui32 MonitoringPort = 0;
    ui32 MonitoringThreads = 0;

    TString DiagnosticsConfig;
    TString RestartsCountFile;
    TString ProfileFile;

    TString LogConfig;
    TString SysConfig;
    TString DomainsConfig;
    TString NameServiceConfig;
    TString DynamicNameServiceConfig;

    TString InterconnectConfig;
    ui32 InterconnectPort = 0;

    TString StorageConfig;
    TString Domain;
    TString SchemeShardDir;
    TString NodeBrokerAddress;
    TString AuthConfig;
    ui32 NodeBrokerPort = 0;
    bool SuppressVersionCheck = false;

    EServiceKind Service = EServiceKind::Null;

    virtual ~TBootstrapOptions() = default;

    void Parse(int argc, char** argv);

    virtual void AddSpecialOptions(NLastGetopt::TOpts& opts);
};

} // NCloud::NFileStore::NDaemon
