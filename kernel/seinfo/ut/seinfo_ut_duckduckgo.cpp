#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc36() {
    // duckduckgo
    {
        TInfo info(SE_DUCKDUCKGO, ST_WEB, "swimmingpool", SF_SEARCH);
        KS_TEST_URL("http://duckduckgo.com/?q=swimmingpool", info);
        KS_TEST_URL("https://duckduckgo.com/?q=swimmingpool", info);
    }
}
