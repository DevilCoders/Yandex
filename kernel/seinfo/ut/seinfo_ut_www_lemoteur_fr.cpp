#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc49() {
    //  www.lemoteur.fr
    {
        TInfo info(SE_LEMOTEUR, ST_WEB, "", SF_SEARCH);
        info.Query = "cow in forest";
        KS_TEST_URL("http://www.lemoteur.fr/?module=lemoteur&bhv=web_fr&kw=cow%20in%20forest", info);
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://www.lemoteur.fr/?module=lemoteur&bhv=images&kw=cow%20in%20forest", info);
        info.Type = ST_VIDEO;
        KS_TEST_URL("http://www.lemoteur.fr/?module=lemoteur&bhv=videos&kw=cow%20in%20forest", info);
        info.Type = ST_NEWS;
        KS_TEST_URL("http://www.lemoteur.fr/?module=lemoteur&bhv=actu&kw=cow%20in%20forest", info);
    }
}
