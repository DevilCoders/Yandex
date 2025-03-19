#pragma once

#include "private.h"

#include <cloud/filestore/libs/daemon/options.h>

namespace NCloud::NFileStore::NServer {

////////////////////////////////////////////////////////////////////////////////

struct TOptions final
    : NDaemon::TBootstrapOptions
{
    TOptions()
    {
        ServerPort = 9022;
        Service = NDaemon::EServiceKind::Local;
    }

    TString ConnectAddress;
    ui32 ConnectPort = 9021;

    void AddSpecialOptions(NLastGetopt::TOpts& opts) override;
};

}   // namespace NCloud::NFileStore::NServer
