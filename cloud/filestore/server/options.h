#pragma once

#include "private.h"

#include <cloud/filestore/libs/daemon/options.h>

#include <util/generic/string.h>

namespace NCloud::NFileStore::NServer {

////////////////////////////////////////////////////////////////////////////////

struct TOptions final
    : NDaemon::TBootstrapOptions
{
    TOptions()
    {
        Service = NDaemon::EServiceKind::Kikimr;
    }

    void AddSpecialOptions(NLastGetopt::TOpts& opts) override;
};

}   // namespace NCloud::NFileStore::NServer
