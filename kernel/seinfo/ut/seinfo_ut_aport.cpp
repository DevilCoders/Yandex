#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc30() {
    // aport
    {
        TInfo info(SE_APORT, ST_WEB, "123", SF_SEARCH);

        KS_TEST_URL("http://www.aport.ru/search/?r=123&That=std", info);

        info.Flags = ESearchFlags(SF_MOBILE | SF_SEARCH);
        KS_TEST_URL("http://m.aport.ru/aport.php?SID=tq4ecfce990e9f8h944&ss=123", info);
    }
}
