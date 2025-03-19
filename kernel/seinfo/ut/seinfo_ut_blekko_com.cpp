#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc43() {
    // blekko.com
    {
        TInfo info(SE_BLEKKO, ST_WEB, "123", SF_SEARCH);

        KS_TEST_URL("http://blekko.com/ws/123", info);

        info.Query = "123 /date";
        KS_TEST_URL("http://blekko.com/ws/123+/date", info);
    }
}
