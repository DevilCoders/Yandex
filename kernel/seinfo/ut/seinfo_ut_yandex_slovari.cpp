#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc45() {
    // yandex.slovari
    {
        TInfo info(SE_YANDEX, ST_ENCYC, "123", SF_SEARCH);
        KS_TEST_URL("http://slovari.yandex.ru/123/%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5/", info);
        KS_TEST_URL("http://slovari.yandex.ru/123/", info);

        info.Query = "winter";
        KS_TEST_URL("http://slovari.yandex.ru/winter/%D0%BF%D0%B5%D1%80%D0%B5%D0%B2%D0%BE%D0%B4/#lingvo/", info);
        KS_TEST_URL("http://slovari.yandex.ru/winter/%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5/", info);
        KS_TEST_URL("http://slovari.yandex.ru/winter/%D0%BF%D1%80%D0%B0%D0%B2%D0%BE%D0%BF%D0%B8%D1%81%D0%B0%D0%BD%D0%B8%D0%B5/", info);

        info.Flags = ESearchFlags(SF_SEARCH | SF_MOBILE);
        KS_TEST_URL("http://m.slovari.yandex.ru/translate.xml?text=winter&lang=en", info);
        KS_TEST_URL("http://m.slovari.yandex.ru/meaning.xml?text=winter", info);
        KS_TEST_URL("http://m.slovari.yandex.ru/spelling.xml?text=winter", info);
    }
}
