#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc69() {
    // popular video hostings
    {
        TInfo info(SE_YOUTUBE, ST_VIDEO, "ford focus", ESearchFlags(SF_SEARCH | SF_LOCAL));
        KS_TEST_URL("http://www.youtube.com/results?search_query=ford+focus&sm=3", info);
        KS_TEST_URL("https://www.youtube.com/results?search_query=ford+focus&sm=3", info);

        info.Flags = ESearchFlags(SF_SEARCH | SF_LOCAL | SF_MOBILE);
        KS_TEST_URL("https://m.youtube.com/results?search_query=ford+focus&sm=3", info);

        info.Name = SE_VIMEO;
        info.Flags = ESearchFlags(SF_SEARCH | SF_LOCAL);
        KS_TEST_URL("http://vimeo.com/search?q=ford+focus", info);
    }
}
