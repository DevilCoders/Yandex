#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc19() {
    // metabot
    {
        TInfo info(SE_METABOT_RU, ST_WEB, "123", SF_SEARCH);
        KS_TEST_URL("http://results.metabot.ru/?st=123&wd=0", info);
    }
}
