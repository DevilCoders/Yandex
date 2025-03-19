#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc33() {
    // gbg.bg
    {
        TInfo info(SE_GBG_BG, ST_CATALOG, "cow in space", SF_SEARCH);

        KS_TEST_URL("http://find.gbg.bg/?q=cow+in+space&c=gbg", info);

        info.Type = ST_WEB;
        KS_TEST_URL("http://search.gbg.bg/?q=cow+in+space&c=google&s=3&lr=", info);

        info.Type = ST_ENCYC;
        KS_TEST_URL("http://find.gbg.bg/?c=znam&q=cow+in+space", info);

        // TODO: parse this
        // http://search.gbg.bg/?c=google&q=cow%20in%20space%20site:vesti.bg
    }
}
