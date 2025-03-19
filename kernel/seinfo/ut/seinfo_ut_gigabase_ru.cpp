#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc38() {
    // gigabase.ru
    {
        TInfo info(SE_GIGABASE_RU, ST_WEB, "123", SF_SEARCH);
        KS_TEST_URL("http://gigabase.ru/search?q=123", info);
    }
}
