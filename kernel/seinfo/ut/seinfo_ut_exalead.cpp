#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc14() {
    // exalead
    {
        TInfo info(SE_EXALEAD, ST_WEB, "cow in space12", SF_SEARCH);

        KS_TEST_URL("http://www.exalead.com/search/web/results/?q=cow+in+space12", info);
        KS_TEST_URL("http://www.exalead.fr/search/web/results/?q=cow+in+space12", info);
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://www.exalead.com/search/image/results/?q=cow+in+space12", info);
        info.Type = ST_VIDEO;
        KS_TEST_URL("http://www.exalead.com/search/video/results/?q=cow+in+space12", info);
        info.Type = ST_ENCYC;
        KS_TEST_URL("http://www.exalead.com/search/wikipedia/results/?q=cow+in+space12", info);
    }
}
