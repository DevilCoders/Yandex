#pragma once

#include "private.h"

#include <util/generic/string.h>

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

struct TOptions
{
    TString VerboseLevel;

    TString MonitoringAddress;
    ui32 MonitoringPort = 0;
    ui32 MonitoringThreads = 0;

    TString ConfigFile;
    TString LogFile;
    TString PidFile;
    TString RecoveryDir;

    void Parse(int argc, char** argv);
};

}   // namespace NCloud::NFileStore::NGateway
