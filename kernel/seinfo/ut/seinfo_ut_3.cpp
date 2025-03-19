#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc3() {
    {
        TInfo info(SE_YANDEX, ST_IMAGES, "пых", SF_SEARCH);
        KS_TEST_URL("http://images.yandex.ru/yandsearch?text=%D0%BF%D1%8B%D1%85", info);

        KS_TEST_URL("http://yandex.com.tr/gorsel/search?text=%D0%BF%D1%8B%D1%85", info);

        KS_TEST_URL("http://yandex.ru/images/search?text=%D0%BF%D1%8B%D1%85", info);

        info.Flags = ESearchFlags(SF_SEARCH | SF_MOBILE);
        KS_TEST_URL("http://yandex.ru/images/pad/search?text=%D0%BF%D1%8B%D1%85", info);
        KS_TEST_URL("http://yandex.ru/images/touch/search?text=%D0%BF%D1%8B%D1%85", info);

        info.Type = ST_VIDEO;
        info.Flags = ESearchFlags(SF_SEARCH);
        KS_TEST_URL("http://video.yandex.ru/search.xml?text=%D0%BF%D1%8B%D1%85", info);
        info.Flags = ESearchFlags(SF_SEARCH | SF_MOBILE);
        KS_TEST_URL("http://yandex.ru/video/pad/search?text=%D0%BF%D1%8B%D1%85", info);
        KS_TEST_URL("http://yandex.ru/video/pad/search?source=tableau_serp&text=%D0%BF%D1%8B%D1%85&redircnt=1443709210.1", info);
        KS_TEST_URL("http://yandex.by/video/touch/search?text=%D0%BF%D1%8B%D1%85", info);

        info.Type = ST_BLOGS;
        info.Flags = ESearchFlags(SF_SEARCH);
        KS_TEST_URL("http://blogs.yandex.ru/search.xml?ft=blog&text=%D0%BF%D1%8B%D1%85", info);

        info.Type = ST_VIDEO;
        info.Query = "winter is coming";
        KS_TEST_URL("http://yandex.com.tr/video/search?text=winter%20is%20coming", info);

        info.Query = "delilo delilo destane dinle";
        KS_TEST_URL("http://yandex.com.tr/video/#!/video/search?filmId=IVH8MFuxUXI&text=delilo%20delilo%20destane%20dinle&path=wizard&where=all", info);
    }
}
