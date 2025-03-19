#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc2() {
    {
        TInfo info(SE_YANDEX, ST_WEB, "", SF_SEARCH);
        info.Query = "семейный поиск";
        KS_TEST_URL("yandex.ru/yandsearch?text=%D1%81%D0%B5%D0%BC%D0%B5%D0%B9%D0%BD%D1%8B%D0%B9+%D0%BF%D0%BE%D0%B8%D1%81%D0%BA&lr=213", info);
        KS_TEST_URL("YANDEX.ru/yandsearch?text=%D1%81%D0%B5%D0%BC%D0%B5%D0%B9%D0%BD%D1%8B%D0%B9+%D0%BF%D0%BE%D0%B8%D1%81%D0%BA&lr=213", info);

        info.Query = "ku";
        KS_TEST_URL("http://yandex.ru/yandsearch?text=ku", info);
        KS_TEST_URL("yandex.ru/yandsearch?text=ku", info);
        KS_TEST_URL("www.yandex.ru/yandsearch?text=ku", info);
        KS_TEST_URL("http://yandex.ru/yandsearch?text=ku", info);

        info.Query = "bmw x5";
        KS_TEST_URL("https://yandex.ru/search/?lr=14&text=bmw+x5", info);

        info.Query = "";
        KS_TEST_URL("http://yandex.ru/familysearch?text=&nl=1&lr=213", info);

        info.Query = "школьницо";
        KS_TEST_URL("http://yandex.ru/schoolsearch?text=%D1%88%D0%BA%D0%BE%D0%BB%D1%8C%D0%BD%D0%B8%D1%86%D0%BE&lr=213", info);
        KS_TEST_URL("yandex.ru/schoolsearch?text=%D1%88%D0%BA%D0%BE%D0%BB%D1%8C%D0%BD%D0%B8%D1%86%D0%BE&lr=213", info);

        info.Query = "astana";
        KS_TEST_URL("yandex.kz/yandsearch?text=astana", info);

        info.Query = "минск";
        KS_TEST_URL("http://yandex.by/yandsearch?text=%D0%BC%D0%B8%D0%BD%D1%81%D0%BA&lr=149", info);

        info.Query = "haifa";
        KS_TEST_URL("https://yandex.co.il/search/?lr=131&text=haifa", info);

        info.Query = "Душанбе";
        KS_TEST_URL("https://yandex.tj/search/?text=%D0%94%D1%83%D1%88%D0%B0%D0%BD%D0%B1%D0%B5&lr=10318", info);

        info.Query = "ульянов александр магнитогорск";
        info.Type = ST_PEOPLE;
        KS_TEST_URL("http://yandex.ru/yandsearch?text=ульянов%20александр%20магнитогорск&lr=213&xjst=1&filter=people", info);

        info.Type = ST_WEB;
        info.Flags = ESearchFlags(SF_SEARCH | SF_MOBILE);
        info.Query = "bmw x7";
        KS_TEST_URL("https://yandex.ru/search/touch/?lr=14&text=bmw+x7", info);
        KS_TEST_URL("https://yandex.ua/search/pad/?lr=143&text=bmw+x7", info);
    }
}
