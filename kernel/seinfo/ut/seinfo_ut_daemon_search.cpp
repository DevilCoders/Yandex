#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc26() {
    // daemon-search
    {
        TInfo info(SE_DAEMON_SEARCH, ST_WEB, "cow in space", SF_SEARCH);

        KS_TEST_URL("http://www.daemon-search.com/rus/explore/web?q=cow+in+space", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://www.daemon-search.com/rus/explore/images?q=cow+in+space", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://www.daemon-search.com/rus/explore/videos?q=cow+in+space", info);
    }
}
