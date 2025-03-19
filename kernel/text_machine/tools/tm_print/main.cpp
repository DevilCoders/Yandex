#include "mode_hits_map.h"

#include <library/cpp/getopt/modchooser.h>


int main(int argc, const char* argv[]) {
    TModChooser chooser;
    chooser.AddMode("hits-map", MainHitsMap<false>, "print document sentences with visible hit positions");
    chooser.AddMode("rhits-map", MainHitsMap<true>, "print requests with visible hit positions");
    chooser.Run(argc, argv);

    return 0;
}
