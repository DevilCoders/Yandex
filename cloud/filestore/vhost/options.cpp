#include "options.h"

#include <library/cpp/getopt/small/last_getopt.h>

#include <util/generic/serialized_enum.h>

namespace NCloud::NFileStore::NServer {

using namespace NLastGetopt;

////////////////////////////////////////////////////////////////////////////////

void TOptions::AddSpecialOptions(NLastGetopt::TOpts& opts)
{
    opts.AddLongOption("connect-address")
        .RequiredArgument("STR")
        .StoreResult(&ConnectAddress);

    opts.AddLongOption("connect-port")
        .RequiredArgument("NUM")
        .StoreResult(&ConnectPort);

    // TODO(fyodor) get rid of this option in favour --app-config in base class
    opts.AddLongOption("vhost-file")
        .RequiredArgument("FILE")
        .StoreResult(&AppConfig);
}

}   // namespace NCloud::NFileStore::NServer
