#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc46() {
    // tineye search by image
    {
        TInfo info(SE_TINEYE, ST_BYIMAGE, "a123", SF_SEARCH);
        KS_TEST_URL("http://www.tineye.com/search/a123/", info);
    }
}
