#include "options.h"

namespace NCloud::NFileStore::NServer
{

void TOptions::AddSpecialOptions(NLastGetopt::TOpts& opts)
{
    // TODO(fyodor) get rid of this option in favour --app-config in base class
    opts.AddLongOption("server-file")
        .RequiredArgument("FILE")
        .StoreResult(&AppConfig);
}

}