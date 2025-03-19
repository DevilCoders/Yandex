#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc58() {
    // yandex xml search. BUKI-1834
    {
        TInfo info(SE_YANDEX, ST_WEB, "nokia", SF_SEARCH);

        KS_TEST_URL("http://xmlsearch.yandex.ru/xmlsearch?text=nokia", info);
    }
}
