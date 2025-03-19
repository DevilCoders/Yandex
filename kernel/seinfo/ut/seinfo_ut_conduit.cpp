#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc24() {
    // conduit
    {
        TInfo info(SE_CONDUIT, ST_WEB, "cow in space", SF_SEARCH);
        KS_TEST_URL("http://search.conduit.com/Results.aspx?q=cow+in+space&meta=all&hl=ru&gl=ru&SelfSearch=1&SearchType=SearchWeb&SearchSource=32&ctid=WEBSITE", info);

        info.Query = "cow";
        info.Type = ST_APPS;
        KS_TEST_URL("http://apps.conduit.com/Search?ctid=WEBSITE&SearchSourceOrigin=32&lang=ru&q=cow", info);
    }
}
