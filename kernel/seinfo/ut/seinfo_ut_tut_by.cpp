#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc20() {
    // tut.by
    {
        TInfo info(SE_TUT_BY, ST_WEB, "партизан", SF_SEARCH);
        KS_TEST_URL("http://search.tut.by/?status=1&ru=1&encoding=1&page=0&how=rlv&query=%D0%BF%D0%B0%D1%80%D1%82%D0%B8%D0%B7%D0%B0%D0%BD", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://search.tut.by/?vs=1&query=%D0%BF%D0%B0%D1%80%D1%82%D0%B8%D0%B7%D0%B0%D0%BD", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://search.tut.by/?is=1&page=0&ru=1&isize=&query=%D0%BF%D0%B0%D1%80%D1%82%D0%B8%D0%B7%D0%B0%D0%BD", info);

        info.Type = ST_NEWS;
        KS_TEST_URL("http://news.tut.by/search/?str=%EF%E0%F0%F2%E8%E7%E0%ED", info);
    }
}
