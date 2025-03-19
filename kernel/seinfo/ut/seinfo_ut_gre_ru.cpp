#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc28() {
    // gre.ru
    {
        TInfo info(SE_GDE_RU, ST_WEB, "можайское шоссе", SF_SEARCH);

        KS_TEST_URL("http://cat.gde.ru//cgi-bin/search.cgi?keywords=%EC%EE%E6%E0%E9%F1%EA%EE%E5%20%F8%EE%F1%F1%E5", info);

        info.Type = ST_COM;
        KS_TEST_URL("http://gde.ru/search.php?search=%EC%EE%E6%E0%E9%F1%EA%EE%E5%20%F8%EE%F1%F1%E5", info);

        info.Type = ST_FORUM;
        KS_TEST_URL("http://forums.gde.ru/search.php?action=search&keywords=%EC%EE%E6%E0%E9%F1%EA%EE%E5%20%F8%EE%F1%F1%E5", info);

        info.Type = ST_ADRESSES;
        KS_TEST_URL("http://adresa.gde.ru/search.php?action=search&keywords=%EC%EE%E6%E0%E9%F1%EA%EE%E5%20%F8%EE%F1%F1%E5", info);

        info.Type = ST_CATALOG;
        KS_TEST_URL("http://top.gde.ru/index.php?sbm_search=&t=%EC%EE%E6%E0%E9%F1%EA%EE%E5%20%F8%EE%F1%F1%E5", info);
    }
}
