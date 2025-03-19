#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc27() {
    // incredimail
    {
        TInfo info(SE_INCREDIMAIL, ST_WEB, "cow in space", SF_SEARCH);
        KS_TEST_URL("http://search.incredimail.com/?q=cow+in+space&lang=russian&source=065316251&gc=ru", info);
    }
}
