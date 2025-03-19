#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc32() {
    // seznam an co.
    {
        TInfo info(SE_SEZNAM_CZ, ST_WEB, "cow in space", SF_SEARCH);

        KS_TEST_URL("http://search.seznam.cz/?aq=-1&oq=cow+in+spa&sourceid=szn-HP&thru=&q=cow+in+space", info);
        info.Type = ST_ENCYC;
        KS_TEST_URL("http://encyklopedie.seznam.cz/search?q=cow%20in%20space", info);

        info.Name = SE_FIRMY_CZ;
        info.Type = ST_COM;
        KS_TEST_URL("http://www.firmy.cz/phr/cow%20in%20space", info);
        info.Name = SE_ZBOZI_CZ;
        KS_TEST_URL("http://www.zbozi.cz/?q=cow%20in%20space", info);

        info.Name = SE_MAPY_CZ;
        info.Type = ST_MAPS;
        KS_TEST_URL("http://www.mapy.cz/#t=s&q=cow%20in%20space", info);

        info.Name = SE_OBRAZKY_CZ;
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://www.obrazky.cz/?q=cow%20in%20space", info);

        // It is translate. Not working now with it
        // http://slovnik.seznam.cz/cz-en/word/?q=cow%20in%20space&id=Xfh5tp6oyO8=
    }
}
