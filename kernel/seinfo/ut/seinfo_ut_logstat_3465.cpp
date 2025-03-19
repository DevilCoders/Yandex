#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc71() {
    // LOGSTAT-3465
    {
        TInfo info(SE_UNKNOWN, ST_VIDEO, "blonde with big tits", ESearchFlags(SF_SEARCH | SF_LOCAL));

        info.Name = SE_SEX_COM;
        KS_TEST_URL("http://www.sex.com/search/pin?query=blonde+with+big+tits", info);
        info.Name = SE_REDTUBE;
        KS_TEST_URL("http://www.redtube.com/?search=blonde+with+big+tits", info);
        info.Name = SE_PORNHUB;
        KS_TEST_URL("http://www.pornhub.com/video/search?search=blonde+with+big+tits", info);
        info.Name = SE_PORN_COM;
        KS_TEST_URL("http://www.porn.com/search.html?q=blonde+with+big+tits", info);
        info.Name = SE_PORNTUBE;
        KS_TEST_URL("http://www.porntube.com/search?q=blonde+with+big+tits", info);
        info.Name = SE_XHAMSTER;
        KS_TEST_URL("http://xhamster.com/search.php?q=blonde+with+big+tits&qcat=video", info);
        info.Name = SE_XVIDEOS;
        KS_TEST_URL("http://www.xvideos.com/?k=blonde+with+big+tits", info);
        info.Name = SE_YOUPORN;
        KS_TEST_URL("http://www.youporn.com/search/?query=blonde+with+big+tits", info);
        info.Name = SE_TUBE8;
        KS_TEST_URL("http://www.tube8.com/searches.html?q=blonde+with+big+tits", info);
        info.Name = SE_XNXX;
        KS_TEST_URL("http://www.xnxx.com/?k=blonde+with+big+tits", info);
        info.Name = SE_PORNMD;
        info.Flags = SF_SEARCH;
        KS_TEST_URL("http://www.pornmd.com/straight/blonde+with+big+tits", info);
    }
}
