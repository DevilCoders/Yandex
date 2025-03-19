#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc70() {
    // LOGSTAT-3295
    {
        TInfo info(SE_GOOGLE, ST_WEB, "123", ESearchFlags(SF_SEARCH));
        KS_TEST_URL("https://www.google.ru/?gfe_rd=cr&ei=rAvdUpLsHISk4ASo2YHwCg#newwindow=1&q=123", info);
    }
}
