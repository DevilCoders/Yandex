#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc72() {
    // LOGSTAT-3580
    {
        TInfo info(SE_SPUTNIK_RU, ST_WEB, "ford focus", SF_SEARCH);

        KS_TEST_URL("http://www.sputnik.ru/search?q=ford+focus", info);
        info.Type = ST_NEWS;
        KS_TEST_URL("http://news.sputnik.ru/search?q=ford%20focus", info);
        info.Type = ST_MAPS;
        KS_TEST_URL("http://maps.sputnik.ru/?q=ford%20focus", info);
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://pics.sputnik.ru/search?q=ford%20focus", info);
        info.Type = ST_VIDEO;
        KS_TEST_URL("http://video.sputnik.ru/search?q=ford%20focus", info);
    }
}
