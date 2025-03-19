#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc80() {
    // LOGSTAT-4211
    {
        TInfo info(SE_GOOGLE, ST_WEB, "28 daysin writing genealogy", ESearchFlags(SF_SEARCH));

        KS_TEST_URL("https://www.google.com/webhp?sourceid=chrome-instant&ion=1&espv=2&ie=UTF-8#q=28%20daysin%20writing%20genealogy", info);
    }
}
