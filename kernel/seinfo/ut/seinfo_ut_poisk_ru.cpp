#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc40() {
    // poisk.ru
    {
        TInfo info(SE_POISK_RU, ST_WEB, "cow in space", SF_SEARCH);
        KS_TEST_URL("http://poisk.ru/cgi-bin/poisk?text=cow+in+space", info);
    }
}
