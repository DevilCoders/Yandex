#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc25() {
    // i.ua
    {
        TInfo info(SE_I_UA, ST_WEB, "cow in space", SF_SEARCH);
        KS_TEST_URL("http://search.i.ua/?q=cow+in+space", info);

        info.Type = ST_CATALOG;
        KS_TEST_URL("http://catalog.i.ua/?q=cow+in+space", info);

        info.Type = ST_COM;
        KS_TEST_URL("http://shop.i.ua/price?q=cow+in+space", info);
        KS_TEST_URL("http://board.i.ua/catalog/?q=cow+in+space", info);

        info.Query = "13";
        info.Flags = ESearchFlags(SF_SEARCH | SF_SOCIAL);
        info.Type = ST_INTERESTS;
        KS_TEST_URL("http://narod.i.ua/interests/?_subm=sform&words=13&trg=interests", info);
        info.Type = ST_BOOKS;
        KS_TEST_URL("http://narod.i.ua/books/?_subm=sform&words=13&trg=books", info);
        info.Type = ST_MUSIC;
        KS_TEST_URL("http://narod.i.ua/musics/?_subm=sform&words=13&trg=musics", info);
        info.Type = ST_VIDEO;
        KS_TEST_URL("http://narod.i.ua/films/?_subm=sform&words=13&trg=films", info);
        info.Type = ST_GAMES;
        KS_TEST_URL("http://narod.i.ua/games/?_subm=sform&words=13&trg=games", info);
        info.Type = ST_PEOPLE;
        KS_TEST_URL("http://narod.i.ua/search/?_subm=sform&words=13&trg=search", info);
    }
}
