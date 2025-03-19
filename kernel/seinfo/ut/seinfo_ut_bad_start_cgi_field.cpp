#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc75() {
    // bad 'start' cgi field
    {
        TInfo info(SE_GOOGLE, ST_WEB, "kanoni sajaro samsaxuris shesaxeb", ESearchFlags(SF_SEARCH));
        info.SetPageStartRaw(10);
        KS_TEST_URL("https://www.google.ge/#q=kanoni+sajaro+samsaxuris+shesaxeb&start=10?s1=3464", info);
    }
}
