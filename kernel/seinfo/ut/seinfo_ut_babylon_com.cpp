#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc31() {
    // babylon.com
    {
        TInfo info(SE_SEARCH_BABYLON, ST_WEB, "cow in space", SF_SEARCH);

        KS_TEST_URL("http://isearch.babylon.com/?q=cow+in+space&babsrc=home&s=web&as=0", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://isearch.babylon.com/?s=images&babsrc=home&q=cow%20in%20space", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://isearch.babylon.com/?s=video&babsrc=home&q=cow%20in%20space", info);

        info.Type = ST_NEWS;
        KS_TEST_URL("http://isearch.babylon.com/?s=news&babsrc=home&q=cow%20in%20space", info);
    }
}
