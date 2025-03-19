#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc4() {
    {
        TInfo info(SE_YANDEX, ST_ENCYC, "123", SF_SEARCH);
        KS_TEST_URL("http://slovari.yandex.ru/123/", info);
        KS_TEST_URL("http://SLOVARI.YANDEX.RU/123/", info);

        info.Type = ST_MAPS;
        KS_TEST_URL("http://maps.yandex.ru/?text=123&sll=37.617671%2C55.755768&sspn=0.833332%2C0.530138&z=10&results=20&ll=37.617671%2C55.755772&spn=1.645203%2C0.751203&l=map", info);

        KS_TEST_URL("http://harita.yandex.com.tr/?text=123&sll=73.365319%2C54.990147&ll=73.378435%2C54.983174&spn=1.084900%2C0.343944&z=11&l=map", info);

        info.Flags = ESearchFlags(SF_SEARCH | SF_MOBILE);
        KS_TEST_URL("http://m.maps.yandex.ru/?text=123&l=map&sll=73.365319%2C54.990147", info);

        info.Flags = SF_SEARCH;
        info.Type = ST_COM;
        KS_TEST_URL("http://market.yandex.ru/search.xml?cvredirect=1&clid=527&text=123", info);
        KS_TEST_URL("https://market.yandex.ru/search.xml?cvredirect=1&clid=527&text=123", info);

        info.Type = ST_CARS;
        KS_TEST_URL("http://auto.yandex.ru/search.xml?text=123", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://fotki.yandex.ru/search.xml?text=123", info);

        info.Type = ST_INTERESTS;
        KS_TEST_URL("http://afisha.yandex.ru/msk/search/?text=123", info);

        info.Name = SE_MOIKRUG;
        info.Type = ST_PEOPLE;
        info.Flags = ESearchFlags(SF_SEARCH | SF_LOCAL | SF_SOCIAL);
        KS_TEST_URL("http://moikrug.ru/persons/?keywords=123&submitted=1", info);
    }
}
