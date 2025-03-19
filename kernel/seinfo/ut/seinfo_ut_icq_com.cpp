#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc41() {
    // icq.com
    {
        TInfo info(SE_ICQ_COM, ST_WEB, "123", SF_SEARCH);

        KS_TEST_URL("http://search.icq.com/search/results.php?q=123&ch_id=hm&search_mode=web", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://search.icq.com/search/img_results.php?q=123&ch_id=", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://video.icq.com/video/video_results.php?q=123&ch_id=", info);
    }
}
