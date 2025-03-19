#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc74() {
    // LOGSTAT-3439
    {
        TInfo info(SE_YANDEX, ST_WEB, "ford focus", SF_SEARCH);

        KS_TEST_URL("http://yandsearch.yandex.ru/yandsearch?clid=1955454&lr=213&msid=22885.30964.1401217596.1417&text=ford+focus", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://yandsearch.yandex.ru/video/search?text=ford%20focus", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://yandsearch.yandex.ru/images/search?text=ford+focus", info);
    }
}
