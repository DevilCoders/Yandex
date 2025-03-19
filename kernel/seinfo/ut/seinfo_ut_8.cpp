#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc8() {
    {
        TInfo info(SE_RAMBLER, ST_WEB, "pepelac", SF_SEARCH);
        KS_TEST_URL("http://nova.rambler.ru/srch?query=pepelac&old_q=%D0%BA%D1%83", info);

        info.Query = "Дойкиком";
        KS_TEST_URL("http://rambler.ru/?query=Дойкиком&btnG=Найти", info);

        info.Query = "ку";
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://nova.rambler.ru/pictures?query=%D0%BA%D1%83", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://nova.rambler.ru/video?query=%D0%BA%D1%83", info);

        info.Query = "12 12";
        info.Type = ST_MAPS;
        KS_TEST_URL("http://maps.rambler.ru/?query=12%2012", info);

        info.Type = ST_WEB;
        KS_TEST_URL("http://nova.rambler.ru/search?btnG=%D0%9D%D0%B0%D0%B9%D1%82%D0%B8%21&query=12+12", info);

        KS_TEST_URL("http://horoscopes.rambler.ru/dreams/?words=12+12&check=Поиск&rubric=0", info);

        info.Query = "12 34";
        info.Type = ST_CATALOG;
        KS_TEST_URL("http://top100.rambler.ru/?query=12+34", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://nova.rambler.ru/video?query=12+34", info);

        info.Query = "1332";
        info.Type = ST_SPORT;
        KS_TEST_URL("http://nova.rambler.ru/search?sort=1&btnG=%CD%E0%E9%F2%E8%21&query=1332&filter=sport.rambler.ru", info);

        info.Query = "332";
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://foto.rambler.ru/search/?btnG=%D0%9D%D0%B0%D0%B9%D1%82%D0%B8%21&query=332", info);

        info.Query = "1";
        info.Type = ST_ENCYC;
        KS_TEST_URL("http://dict.rambler.ru/?coll=4.0er&query=1", info);

        info.Flags = ESearchFlags(SF_MOBILE | SF_SEARCH);

        info.Query = "12332";
        info.Type = ST_WEB;
        KS_TEST_URL("http://m.rambler.ru/search/?query=12332", info);

        info.Query = "рамблер";
        KS_TEST_URL("http://m.search.rambler.ru/search?query=рамблер", info);

        info.Query = "123";
        info.Type = ST_NEWS;
        KS_TEST_URL("http://m.rambler.ru/news/search/?query=123", info);

        info.Query = "1233";
        info.Type = ST_MUSIC;
        KS_TEST_URL("http://m.rambler.ru/premium-music/search/?PHPSESSID=d7f4945afd36fdfe45c1dd109b992ca6&t=1233", info);

        info.Query = "ford focus";
        info.Type = ST_COM;
        info.Flags = ESearchFlags(SF_SEARCH);
        KS_TEST_URL("http://supermarket.rambler.ru/search/?0=&query=ford%20focus", info);
    }
}
