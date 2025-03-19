#include "options.h"

#include <cloud/filestore/libs/daemon/options.h>

#include <util/generic/serialized_enum.h>

namespace NCloud::NFileStore::NDaemon
{

void TBootstrapOptions::Parse(int argc, char** argv)
{
    NLastGetopt::TOpts opts;
    opts.AddHelpOption('h');
    opts.AddVersionOption();

    opts.AddLongOption("server-address")
        .RequiredArgument("STR")
        .StoreResult(&ServerAddress);

    opts.AddLongOption("server-port")
        .RequiredArgument("NUM")
        .StoreResult(&ServerPort);

    opts.AddLongOption("log-file")
        .RequiredArgument("FILE")
        .StoreResult(&LogConfig);

    opts.AddLongOption("sys-file")
        .RequiredArgument("FILE")
        .StoreResult(&SysConfig);

    opts.AddLongOption("domains-file")
        .RequiredArgument("FILE")
        .StoreResult(&DomainsConfig);

    opts.AddLongOption("naming-file")
        .RequiredArgument("FILE")
        .StoreResult(&NameServiceConfig);

    opts.AddLongOption("dynamic-naming-file")
        .RequiredArgument("FILE")
        .StoreResult(&DynamicNameServiceConfig);

    opts.AddLongOption("auth-file")
        .RequiredArgument("FILE")
        .StoreResult(&AuthConfig);

    opts.AddLongOption("ic-file")
        .RequiredArgument("PATH")
        .StoreResult(&InterconnectConfig);

    opts.AddLongOption("ic-port")
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

    opts.AddLongOption("no-mem-lock")
        .NoArgument()
        .StoreTrue(&NoMemLock);

    opts.AddLongOption("storage-file")
        .RequiredArgument("FILE")
        .StoreResult(&StorageConfig);

    opts.AddLongOption("domain")
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

    opts.AddLongOption("app-conifg")
        .RequiredArgument("FILE")
        .StoreResult(&AppConfig);

    opts.AddLongOption("diag-file")
        .RequiredArgument("FILE")
        .StoreResult(&DiagnosticsConfig);

    opts.AddLongOption("restarts-count-file")
        .RequiredArgument("PATH")
        .StoreResult(&RestartsCountFile);

    opts.AddLongOption("profile-file")
        .RequiredArgument("PATH")
        .StoreResult(&ProfileFile);

    opts.AddLongOption("suppress-version-check", "Suppress version compatibility checking via IC")
        .NoArgument()
        .StoreTrue(&SuppressVersionCheck);

    opts.AddLongOption("service")
        .RequiredArgument("{" + GetEnumAllNames<EServiceKind>() + "}")
        .Handler1T<TString>([this] (const auto& s) {
            Service = FromString<EServiceKind>(s);
        });

    const auto& verbose = opts.AddLongOption("verbose")
        .OptionalArgument("STR")
        .StoreResult(&VerboseLevel);

    AddSpecialOptions(opts);

    NLastGetopt::TOptsParseResultException res(&opts, argc, argv);

    if (res.Has(&verbose) && !VerboseLevel) {
        VerboseLevel = "debug";
    }
}

void TBootstrapOptions::AddSpecialOptions(NLastGetopt::TOpts& opts)
{
    // Do nothing by default, override in children if need more options
    Y_UNUSED(opts);
}

} // NCloud::NFileStore::NDaemon
