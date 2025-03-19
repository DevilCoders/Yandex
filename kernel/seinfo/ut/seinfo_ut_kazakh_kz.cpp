#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc35() {
    // kazakh.kz
    {
        TInfo info(SE_KAZAKH_RU, ST_WEB, "cow in shop", SF_SEARCH);

        KS_TEST_URL("http://kazakh.ru/search/?query=cow+in+shop&wheresearch=3", info);
        info.Flags = ESearchFlags(SF_LOCAL | SF_SEARCH);
        KS_TEST_URL("http://kazakh.ru/search/?query=cow+in+shop&wheresearch=1", info);

        info.Type = ST_NEWS;
        KS_TEST_URL("http://www.kazakh.ru/news/search/?query=cow+in+shop&wheresearch=1", info);
        KS_TEST_URL("http://www.kazakh.ru/news/search/?wheresearch=1&query=cow+in+shop", info);

        info.Type = ST_FORUM;
        KS_TEST_URL("http://www.kazakh.ru/talk/search/?wheresearch=1&query=cow+in+shop", info);

        info.Flags = SF_SEARCH;

        info.Type = ST_BLOGS;
        KS_TEST_URL("http://blogs.kazakh.ru/search/?page=search&q=cow+in+shop", info);

        info.Type = ST_CATALOG;
        KS_TEST_URL("http://catalog.kazakh.ru/search/search.php?s=Y&q=cow+in+shop", info);
    }
}
