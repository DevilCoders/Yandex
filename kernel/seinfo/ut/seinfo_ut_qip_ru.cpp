#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc16() {
    // qip.ru
    {
        TInfo info(SE_QIP, ST_WEB, "ЛДПР выборы 2011", SF_SEARCH);

        KS_TEST_URL("http://magna.qip.ru/?q=%D0%9B%D0%94%D0%9F%D0%A0+%D0%B2%D1%8B%D0%B1%D0%BE%D1%80%D1%8B+2011&from=searchqip", info);
        KS_TEST_URL("http://search.qip.ru/search?query=%D0%9B%D0%94%D0%9F%D0%A0+%D0%B2%D1%8B%D0%B1%D0%BE%D1%80%D1%8B+2011", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://search.photo.qip.ru/search?query=%D0%9B%D0%94%D0%9F%D0%A0%20%D0%B2%D1%8B%D0%B1%D0%BE%D1%80%D1%8B%202011", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://search.video.qip.ru/?query=%D0%9B%D0%94%D0%9F%D0%A0%20%D0%B2%D1%8B%D0%B1%D0%BE%D1%80%D1%8B%202011", info);

        info.Type = ST_MAPS;
        KS_TEST_URL("http://maps.qip.ru/?query=%D0%9B%D0%94%D0%9F%D0%A0+%D0%B2%D1%8B%D0%B1%D0%BE%D1%80%D1%8B+2011&from=searchqip", info);

        info.Type = ST_ABSTRACTS;
        KS_TEST_URL("http://5ballov.qip.ru/referats/search/?query=%CB%C4%CF%D0+%E2%FB%E1%EE%F0%FB+2011", info);

        info.Type = ST_EVENTS;
        KS_TEST_URL("http://afisha.qip.ru/search/?mainCategoryId=&search=%D0%9B%D0%94%D0%9F%D0%A0+%D0%B2%D1%8B%D0%B1%D0%BE%D1%80%D1%8B+2011", info);

        info.Type = ST_JOB;
        KS_TEST_URL("http://job.qip.ru/vac.shtml?words=%CB%C4%CF%D0+%E2%FB%E1%EE%F0%FB+2011&where=", info);

        info.Type = ST_WEB;
        info.Flags = ESearchFlags(SF_MOBILE | SF_SEARCH);
        KS_TEST_URL("http://pda.search.qip.ru/pda/search?query=%D0%9B%D0%94%D0%9F%D0%A0+%D0%B2%D1%8B%D0%B1%D0%BE%D1%80%D1%8B+2011", info);
    }
}
