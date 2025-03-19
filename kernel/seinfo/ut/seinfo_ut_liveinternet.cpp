#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc63() {
    // liveinternet
    {
        TInfo info(SE_LIVEINTERNET, ST_WEB, "ford focus", SF_SEARCH);

        KS_TEST_URL("http://www.liveinternet.ru/q/?q=ford+focus&t=web", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://www.liveinternet.ru/q/?t=img&q=ford%20focus", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://www.liveinternet.ru/q/?t=video&q=ford%20focus", info);

        info.Type = ST_COM;
        KS_TEST_URL("http://www.liveinternet.ru/market/?findtext=ford%20focus", info);

        info.Type = ST_NEWS;
        KS_TEST_URL("http://news.liveinternet.ru/?search=ford%20focus", info);

        info.Flags = ESearchFlags(SF_SOCIAL | SF_SEARCH);

        info.Type = ST_WEB;
        KS_TEST_URL("http://www.liveinternet.ru/search/?q=ford%20focus", info);

        info.Type = ST_PEOPLE;
        KS_TEST_URL("http://www.liveinternet.ru/jsearch/?s=ford%20focus", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://www.liveinternet.ru/msearch/?q=ford%20focus&i=3", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://www.liveinternet.ru/msearch/?q=ford%20focus&i=2", info);

        info.Type = ST_MUSIC;
        KS_TEST_URL("http://www.liveinternet.ru/msearch/?q=ford%20focus&i=1", info);
    }
}
