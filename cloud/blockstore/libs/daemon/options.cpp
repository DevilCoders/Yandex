#include "options.h"

#include <library/cpp/getopt/small/last_getopt.h>

#include <util/generic/serialized_enum.h>

namespace NCloud::NBlockStore::NServer {

using namespace NLastGetopt;

////////////////////////////////////////////////////////////////////////////////

void TOptions::Parse(int argc, char** argv)
{
    TOpts opts;
    opts.AddHelpOption();

    opts.AddLongOption("service", "service to run")
        .RequiredArgument("{" + GetEnumAllNames<EServiceKind>() + "}")
        .DefaultValue(ToString(EServiceKind::Kikimr))
        .Handler1T<TString>([this] (const auto& s) {
            ServiceKind = FromString<EServiceKind>(s);
        });

    opts.AddLongOption("log-file")
        .RequiredArgument("FILE")
        .StoreResult(&LogConfig);

    opts.AddLongOption("sys-file")
        .RequiredArgument("FILE")
        .StoreResult(&SysConfig);

    opts.AddLongOption("domains-file")
        .RequiredArgument("FILE")
        .StoreResult(&DomainsConfig);

    opts.AddLongOption("ic-file")
        .RequiredArgument("PATH")
        .StoreResult(&InterconnectConfig);

    const auto& icPort = opts.AddLongOption("ic-port")
        .RequiredArgument("NUM")
        .StoreResult(&InterconnectPort);

    opts.AddLongOption("mon-file")
        .RequiredArgument("PATH")
        .StoreResult(&MonitoringConfig);

    opts.AddLongOption("mon-address")
        .RequiredArgument()
        .StoreResult(&MonitoringAddress);

    opts.AddLongOption("mon-port")
        .RequiredArgument("NUM")
        .StoreResult(&MonitoringPort);

    opts.AddLongOption("mon-threads")
        .RequiredArgument("NUM")
        .StoreResult(&MonitoringThreads);

    const auto& domain = opts.AddLongOption("domain")
        .RequiredArgument("STR")
        .StoreResult(&Domain);

    opts.AddLongOption("scheme-shard-dir")
        .RequiredArgument("STR")
        .StoreResult(&SchemeShardDir);

    opts.AddLongOption("node-broker")
        .RequiredArgument("STR:NUM")
        .StoreResult(&NodeBrokerAddress);

    opts.AddLongOption("node-broker-port")
        .RequiredArgument("PORT")
        .StoreResult(&NodeBrokerPort);

    opts.AddLongOption("server-file")
        .RequiredArgument("FILE")
        .StoreResult(&ServerConfig);

    opts.AddLongOption("server-port")
        .RequiredArgument("NUM")
        .StoreResult(&ServerPort);

    opts.AddLongOption("data-server-port")
        .RequiredArgument("NUM")
        .StoreResult(&DataServerPort);

    opts.AddLongOption("secure-server-port")
        .RequiredArgument("NUM")
        .StoreResult(&SecureServerPort);

    opts.AddLongOption("client-file")
        .RequiredArgument("FILE")
        .StoreResult(&EndpointConfig);

    opts.AddLongOption("storage-file")
        .RequiredArgument("FILE")
        .StoreResult(&StorageConfig);

    opts.AddLongOption("disk-agent-file")
        .RequiredArgument("FILE")
        .StoreResult(&DiskAgentConfig);

    opts.AddLongOption("local-storage-file")
        .RequiredArgument("FILE")
        .StoreResult(&LocalStorageConfig);

    opts.AddLongOption("dr-proxy-file")
        .RequiredArgument("FILE")
        .StoreResult(&DiskRegistryProxyConfig);

    opts.AddLongOption("naming-file")
        .RequiredArgument("FILE")
        .StoreResult(&NameServiceConfig);

    opts.AddLongOption("dynamic-naming-file")
        .RequiredArgument("FILE")
        .StoreResult(&DynamicNameServiceConfig);

    opts.AddLongOption("auth-file")
        .RequiredArgument("FILE")
        .StoreResult(&AuthConfig);

    opts.AddLongOption("diag-file")
        .RequiredArgument("FILE")
        .StoreResult(&DiagnosticsConfig);

    opts.AddLongOption("ydbstats-file")
        .RequiredArgument("FILE")
        .StoreResult(&StatsUploadConfig);

    opts.AddLongOption("discovery-file")
        .RequiredArgument("FILE")
        .StoreResult(&DiscoveryConfig);

    opts.AddLongOption("features-file")
        .RequiredArgument("FILE")
        .StoreResult(&FeaturesConfig);

    opts.AddLongOption("restarts-count-file")
        .RequiredArgument("PATH")
        .StoreResult(&RestartsCountFile);

    opts.AddLongOption("metering-file")
        .RequiredArgument("PATH")
        .StoreResult(&MeteringFile);

    opts.AddLongOption("profile-file")
        .RequiredArgument("PATH")
        .StoreResult(&ProfileFile);

    opts.AddLongOption("location-file")
        .RequiredArgument("PATH")
        .StoreResult(&LocationFile);

    opts.AddLongOption("logbroker-file")
        .RequiredArgument("PATH")
        .StoreResult(&LogbrokerConfig);

    opts.AddLongOption("notify-file")
        .RequiredArgument("PATH")
        .StoreResult(&NotifyConfig);

    const auto& verbose = opts.AddLongOption("verbose", "output level for diagnostics messages")
        .OptionalArgument("STR")
        .StoreResult(&VerboseLevel);

    opts.AddLongOption("grpc-trace", "turn on grpc tracing")
        .NoArgument()
        .StoreTrue(&EnableGrpcTracing);

    opts.AddLongOption("lock-memory", "lock process memory after initialization")
        .NoArgument()
        .StoreTrue(&MemLock);

    opts.AddLongOption("load-configs-from-cms", "load configs from CMS")
        .NoArgument()
        .StoreTrue(&LoadCmsConfigs);

    opts.AddLongOption("temporary-server", "run temporary server for blue-green deployment")
        .NoArgument()
        .StoreTrue(&TemporaryServer);

    opts.AddLongOption("suppress-version-check", "Suppress version compatibility checking via IC")
        .NoArgument()
        .StoreTrue(&SuppressVersionCheck);

    TOptsParseResultException res(&opts, argc, argv);

    if (res.Has(&verbose) && !VerboseLevel) {
        VerboseLevel = "debug";
    }

    if (ServiceKind == EServiceKind::Kikimr) {
        Y_ENSURE(res.Has(&icPort), "'--ic-port' option is required for kikimr service");
        Y_ENSURE(res.Has(&domain), "'--domain' option is required for kikimr service");

        Y_ENSURE(SysConfig && DomainsConfig || LoadCmsConfigs,
            "sys-file and domains-file options are required if load-configs-from-cms is not set");
    }
}

}   // namespace NCloud::NBlockStore::NServer
