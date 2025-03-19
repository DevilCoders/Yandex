#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc18() {
    // webalta
    {
        TInfo info(SE_WEBALTA, ST_WEB, "корова", SF_SEARCH);
        KS_TEST_URL("http://webalta.ru/search?q=%D0%BA%D0%BE%D1%80%D0%BE%D0%B2%D0%B0", info);
    }
}
