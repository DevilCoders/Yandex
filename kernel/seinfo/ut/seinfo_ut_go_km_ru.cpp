#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc37() {
    // go.km.ru
    {
        TInfo info(SE_GO_KM_RU, ST_WEB, "to be or not to be", SF_SEARCH);

        KS_TEST_URL("http://go.km.ru/?sq=to+be+or+not+to+be&idr=2", info);
        info.Flags = ESearchFlags(SF_LOCAL | SF_SEARCH);
        KS_TEST_URL("http://go.km.ru/?sq=to+be+or+not+to+be&idr=1", info);
    }
}
