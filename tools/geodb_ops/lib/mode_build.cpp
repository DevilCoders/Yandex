#include "modes.h"

#include <library/cpp/getopt/small/modchooser.h>

int NGeoDBOps::MainBuild(const int argc, const char* argv[]) {
    TModChooser modChooser;
    modChooser.SetDescription("Build GeoDB");
    modChooser.AddMode(
        "file",
        NGeoDBOps::MainBuildFile,
        "Build from geobase JSON dump");
    modChooser.AddMode(
        "remote",
        NGeoDBOps::MainBuildRemote,
        "Build from geobase HTTP-JSON export");
    return modChooser.Run(argc, argv);
}
