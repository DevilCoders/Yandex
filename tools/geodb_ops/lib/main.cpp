#include "modes.h"

#include <library/cpp/getopt/small/modchooser.h>

int NGeoDBOps::Main(const int argc, const char* argv[]) {
    TModChooser modChooser;
    modChooser.SetDescription("Tool for GeoDB building and printing");
    modChooser.AddMode(
        "build",
        NGeoDBOps::MainBuild,
        "Build GeoDB");
    modChooser.AddMode(
        "print",
        NGeoDBOps::MainPrint,
        "Print GeoDB");
    modChooser.AddMode(
        "validate",
        NGeoDBOps::MainValidate,
        "Validate GeoDB");
    return modChooser.Run(argc, argv);
}
