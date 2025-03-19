#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc15() {
    // alexa
    {
        TInfo info(SE_ALEXA, ST_WEB, "cow in space", SF_SEARCH);
        KS_TEST_URL("http://www.alexa.com/search?q=cow+in+space&r=home_home&p=bigtop", info);
    }
}
