#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc44() {
    // topsy
    {
        TInfo info(SE_TOPSY, ST_WEB, "123456", SF_SEARCH);

        KS_TEST_URL("http://topsy.com/s?q=123456", info);

        info.Type = ST_SOCIAL;
        KS_TEST_URL("http://topsy.com/s/123456/tweet", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://topsy.com/s/123456/image", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://topsy.com/s/123456/video", info);

        info.Type = ST_PEOPLE;
        KS_TEST_URL("http://topsy.com/s/123456/expert", info);
    }
}
