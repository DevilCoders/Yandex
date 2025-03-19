#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc34() {
    // nur.kz
    {
        TInfo info(SE_NUR_KZ, ST_WEB, "cow in space", SF_SEARCH);

        KS_TEST_URL("http://search.nur.kz/?status=1&ru=1&encoding=1&page=0&how=rlv&query=cow+in+space", info);

        info.Type = ST_NEWS;
        KS_TEST_URL("http://news.nur.kz/search/?str=cow+in+space", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://video.nur.kz/video&search?q=cow+in+space&charset=utf-8", info);

        info.Type = ST_MUSIC;
        KS_TEST_URL("http://music.nur.kz/search&?search=1&url=1&q=cow+in+space&charset=utf-8", info);

        info.Type = ST_BLOGS;
        KS_TEST_URL("http://blog.nur.kz/?s=cow+in+space", info);
    }
}
