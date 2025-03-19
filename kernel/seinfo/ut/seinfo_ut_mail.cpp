#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc10() {
    // mail
    {
        TInfo info(SE_MAIL, ST_WEB, "хуй", SF_SEARCH);

        KS_TEST_URL("http://go.mail.ru/search?q=%F5%F3%E9", info);

        info.Query = "";
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://go.mail.ru/search_images?q=", info);

        info.Query = "хуй";
        info.Type = ST_VIDEO;
        KS_TEST_URL("http://go.mail.ru/search_video?q=%F5%F3%E9", info);

        info.Query = "cow in forest";
        info.Type = ST_WEB;
        KS_TEST_URL("http://go.mail.ru/search?rch=l&q=cow+in+forest", info);
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://go.mail.ru/search_images?q=cow%20in%20forest", info);
        info.Type = ST_VIDEO;
        KS_TEST_URL("http://go.mail.ru/search_video?q=cow%20in%20forest", info);

        info.Query = "путин";
        info.Type = ST_BLOGS;
        KS_TEST_URL("http://go.mail.ru/realtime?q=%D0%BF%D1%83%D1%82%D0%B8%D0%BD&tf=0#tf=0", info);

        info.Query = "123";
        info.Type = ST_WEB;
        info.Flags = ESearchFlags(SF_MOBILE | SF_SEARCH);
        KS_TEST_URL("http://go.mail.ru/msearch?q=123", info);
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://go.mail.ru/mimages?q=123&rch=l", info);
        info.Type = ST_ANSWER;
        KS_TEST_URL("http://go.mail.ru/manswer?q=123&rch=l", info);

        info.Query = "путин";
        info.Type = ST_ENCYC;
        info.Flags = SF_SEARCH;
        KS_TEST_URL("http://search.enc.mail.ru/search_enc?q=%D0%BF%D1%83%D1%82%D0%B8%D0%BD", info);
        info.Type = ST_COM;
        KS_TEST_URL("http://torg.mail.ru/search/?search_=%EF%F3%F2%E8%ED&q=%EF%F3%F2%E8%ED", info);
    }
}
