#pragma once

#include <util/generic/string.h>

namespace NAntiRobot {
    struct TCommandLineParams {
        TString ConfigFileName;
        TString BaseDirName;
        TString LogsDirName;
        ui16 Port;
        bool DebugMode;
        bool NoTVMClient;

        TCommandLineParams()
            : Port(0)
            , DebugMode(false)
            , NoTVMClient(false)
        {
        }
    };
}
