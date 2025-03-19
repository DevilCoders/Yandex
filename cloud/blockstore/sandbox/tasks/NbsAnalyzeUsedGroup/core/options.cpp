#include "options.h"

#include <library/cpp/getopt/small/last_getopt.h>

namespace NCloud::NBlockStore::NAnalyzeUsedGroup {

////////////////////////////////////////////////////////////////////////////////

void TOptions::Parse(int argc, char** argv)
{
    NLastGetopt::TOpts opts;
    opts.AddHelpOption();

    opts.AddLongOption("token", "OAuth token for connect to YDB")
        .OptionalArgument("FILE")
        .StoreResult(&Token);

    opts.AddLongOption("endpoint", "URL to YDB")
        .OptionalArgument("URL")
        .StoreResult(&Endpoint);

    opts.AddLongOption("database", "Database name for take data")
        .OptionalArgument("NAME")
        .StoreResult(&Database);

    opts.AddLongOption("table", "Table name for take data")
        .OptionalArgument("NAME")
        .StoreResult(&Table);

    opts.AddLongOption("workers", "Count of executable thread")
        .OptionalArgument("COUNT")
        .StoreResult(&ThreadCount, 1);

    NLastGetopt::TOptsParseResultException res(&opts, argc, argv);
}

}   // NCloud::NBlockStore::NAnalyzeUsedGroup
