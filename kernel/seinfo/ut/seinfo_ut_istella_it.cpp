#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc76() {
    // istella.it
    {
        TInfo info(SE_ISTELLA, ST_WEB, "ford focus", SF_SEARCH);

        KS_TEST_URL("http://istella.it/search/?key=ford+focus", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://image.istella.it/search/?key=ford+focus&social=0", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://video.istella.it/search/?key=ford+focus&social=0", info);

        info.Type = ST_NEWS;
        KS_TEST_URL("http://news.istella.it/search/?key=ford+focus", info);
    }
}
