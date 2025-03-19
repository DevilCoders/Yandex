#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc68() {
    //LOGSTAT-2923
    {
        TInfo info(SE_YANDEX, ST_VIDEO, "ford focus", SF_SEARCH);
        KS_TEST_URL("http://yandex.ru/video/search?text=ford%20focus", info);
        KS_TEST_URL("https://yandex.ru/video/search?text=ford%20focus", info);
        KS_TEST_URL("http://yandex.com.tr/video/search?text=ford%20focus", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://yandex.com.tr/gorsel/search?text=ford%20focus&uinfo=sw-1600-sh-1200-ww-1538-wh-1069-wp-4x3_1600x1200", info);
    }
}
