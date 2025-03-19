#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc11() {
    // gogo
    {
        TInfo info(SE_GOGO, ST_WEB, "ку", SF_SEARCH);

        KS_TEST_URL("www.gogo.ru/go?q=%EA%F3", info);

        info.Query = "шабанов";
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://www.gogo.ru/images?q=%F8%E0%E1%E0%ED%EE%E2&is=", info);

        info.Query = "школьницы";
        info.Type = ST_VIDEO;
        KS_TEST_URL("gogo.ru/video?q=%F8%EA%EE%EB%FC%ED%E8%F6%FB", info);
    }
}
