#include "options.h"

#include <library/cpp/getopt/small/last_getopt.h>

namespace NCloud::NFileStore::NGateway {

using namespace NLastGetopt;

////////////////////////////////////////////////////////////////////////////////

void TOptions::Parse(int argc, char** argv)
{
    TOpts opts;
    opts.AddHelpOption('h');
    opts.AddVersionOption();

    auto verbose = opts.AddLongOption("verbose")
        .OptionalArgument("STR")
        .StoreResult(&VerboseLevel);

    opts.AddLongOption("mon-address")
        .RequiredArgument("STR")
        .StoreResult(&MonitoringAddress);

    opts.AddLongOption("mon-port")
        .RequiredArgument("NUM")
        .StoreResult(&MonitoringPort);

    opts.AddLongOption("mon-threads")
        .RequiredArgument("NUM")
        .StoreResult(&MonitoringThreads);

    opts.AddLongOption('c', "config-file")
        .RequiredArgument("STR")
        .StoreResult(&ConfigFile);

    opts.AddLongOption('l', "log-file")
        .RequiredArgument("STR")
        .StoreResult(&LogFile);

    opts.AddLongOption('p', "pid-file")
        .RequiredArgument("STR")
        .StoreResult(&PidFile);

    opts.AddLongOption('r', "recovery-dir")
        .RequiredArgument("STR")
        .StoreResult(&RecoveryDir);

    TOptsParseResultException res(&opts, argc, argv);

    if (res.Has(&verbose) && !VerboseLevel) {
        VerboseLevel = "debug";
    }
}

}   // namespace NCloud::NFileStore::NGateway
