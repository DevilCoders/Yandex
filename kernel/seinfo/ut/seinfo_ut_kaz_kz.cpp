#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc39() {
    // kaz.kz
    {
        TInfo info(SE_SEARCH_KAZ_KZ, ST_WEB, "cow in space", SF_SEARCH);
        KS_TEST_URL("http://search.kaz.kz/search.cgi?i=&q=cow+in+space&c=&t=0&m=all&f=0&fm=0&eq=1&syn=0&ps=10", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://search.kaz.kz/search.cgi?i=1&q=cow+in+space&c=&t=0&m=all&f=0&fm=0&eq=1&syn=1&ps=10", info);

        info.Type = ST_NEWS;
        KS_TEST_URL("http://search.kaz.kz/search.cgi?i=2&q=cow+in+space&c=&t=0&m=all&f=0&fm=0&eq=1&syn=1&ps=10", info);
    }
}
