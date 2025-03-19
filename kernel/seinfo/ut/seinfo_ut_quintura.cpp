#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc17() {
    // quintura
    {
        TInfo info(SE_QUINTURA, ST_WEB, "123", SF_SEARCH);

        KS_TEST_URL("http://quintura.com/?request=123&tab=0&page=1&tabid=", info);
        info.Query = "123 музыка";
        KS_TEST_URL("http://quintura.com/?request=123+%D0%BC%D1%83%D0%B7%D1%8B%D0%BA%D0%B0&tab=0&page=1&tabid=", info);
    }
}
