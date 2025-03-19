#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc5() {
    {
        TInfo info(SE_YANDEX, ST_NEWS, "cow in space", SF_SEARCH);
        KS_TEST_URL("http://news.yandex.ru/yandsearch?text=cow+in+space&rpt=nnews2&grhow=clutop", info);

        info.Query = "winter";
        KS_TEST_URL("http://haber.yandex.com.tr/yandsearch?text=winter&rpt=nnews2&grhow=cluto", info);

        info.Query = "lenta";
        KS_TEST_URL("https://newssearch.yandex.ru/news/search?text=lenta", info);

        info.Query = "путин";
        info.Flags = ESearchFlags(SF_SEARCH | SF_MOBILE);
        KS_TEST_URL("https://m.newssearch.yandex.by/news/search?text=%D0%BF%D1%83%D1%82%D0%B8%D0%BD", info);

        info.SetPageSizeRaw(20);
        KS_TEST_URL("http://m.news.yandex.ru/yandsearch?rpt=nnews2&grhow=clutop&numdoc=20&text=%D0%BF%D1%83%D1%82%D0%B8%D0%BD", info);
        KS_TEST_URL("http://pda.news.yandex.ru/yandsearch?rpt=nnews2&grhow=clutop&numdoc=20&text=%D0%BF%D1%83%D1%82%D0%B8%D0%BD", info);
    }
}
