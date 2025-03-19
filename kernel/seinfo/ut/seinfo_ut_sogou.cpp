#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc23() {
    // sogou
    {
        TInfo info(SE_SOGOU, ST_WEB, "cats", SF_SEARCH);
        KS_TEST_URL("http://www.sogou.com/web?query=cats&p=&w=03021800", info);

        info.Type = ST_NEWS;
        KS_TEST_URL("http://news.sogou.com/news?ie=utf8&p=40230447&interV=kKIOkrELjbgLmLkElbYTkKIKmbELjboJmLkEkL8TkKIMkLELjbcQmLkEmrELjbgRmLkEkLYTkKIMlo==_-1376535426&query=cats", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://pic.sogou.com/pics?query=cats&p=&w=03021800", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://v.sogou.com/v?query=cats&p=&w=", info);
    }
}
