#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc48() {
    // case sensitiveness
    {
        TInfo info(SE_YANDEX, ST_WEB, "", SF_SEARCH);
        info.Query = "CAT";
        KS_TEST_URL("http://yandex.ru/yandsearch?text=CAT", info);
        info.Query = "cat";
        KS_TEST_URL_CASELESS("http://yandex.ru/yandsearch?text=CAT", info);
    }
}
