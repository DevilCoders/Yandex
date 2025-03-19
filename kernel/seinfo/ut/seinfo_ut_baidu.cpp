#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc6() {
    // baidu
    {
        TInfo info(SE_BAIDU, ST_WEB, "cats", SF_SEARCH);
        KS_TEST_URL("http://www.baidu.com/s?ie=utf-8&wd=cats", info);

        info.Type = ST_NEWS;
        KS_TEST_URL("http://news.baidu.com/ns?cl=2&rn=20&tn=news&word=cats&ie=utf-8", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://image.baidu.com/i?tn=baiduimage&ct=201326592&lm=-1&cl=2&t=12&word=cats&ie=utf-8&fr=news", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://v.baidu.com/v?ct=301989888&s=25&ie=utf-8&word=cats", info);
    }
}
