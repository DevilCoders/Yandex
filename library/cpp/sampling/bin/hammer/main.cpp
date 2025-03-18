#include "modes.h"

#include <library/cpp/getopt/small/modchooser.h>

int main(const int argc, const char* argv[]) {
    auto&& modChooser = TModChooser{};
    modChooser.SetDescription("Find reformulated queries");
    modChooser.AddMode(
        "random",
        MainRandom,
        "Generate random data");
    modChooser.AddMode(
        "load",
        MainLoad,
        "Load data from file");
    return modChooser.Run(argc, argv);
}
