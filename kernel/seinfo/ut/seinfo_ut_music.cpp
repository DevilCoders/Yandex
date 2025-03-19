#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc53() {
    {
        TInfo info(SE_ODNOKLASSNIKI, ST_SOCIAL, "winter", ESearchFlags(SF_SOCIAL | SF_SEARCH | SF_LOCAL));
        KS_TEST_URL("http://odnoklassniki.ru/dk?st.cmd=searchResult&st.query=winter", info);
        info.Name = SE_GOOGLE;
        info.Query = "winter is coming";
        KS_TEST_URL("https://plus.google.com/u/0/s/winter%20is%20coming?hl=ru", info);
        info.Name = SE_LINKEDIN;
        info.Query = "ivan petrov";
        KS_TEST_URL("http://www.linkedin.com/search/fpsearch?type=people&keywords=ivan+petrov&pplSearchOrigin=GLHD&pageKey=nprofile_v2_view_fs", info);
        info.Query = "winter";
        info.Name = SE_MAIL;
        info.Flags = ESearchFlags(SF_SOCIAL | SF_SEARCH | SF_LOCAL);
        info.Type = ST_MUSIC;
        KS_TEST_URL("http://my.mail.ru/bk/example/audio?search=winter&st=search&head=1", info);
        info.Type = ST_VIDEO;
        KS_TEST_URL("http://my.mail.ru/cgi-bin/video/search?q=winter", info);
        info.Type = ST_SOCIAL;
        KS_TEST_URL("http://my.mail.ru/my/search_people?q=winter&st=search&head=1&search=%CD%E0%E9%F2%E8", info);
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://foto.mail.ru/cgi-bin/photo/search?q=winter&st=search&head=1&search=%CD%E0%E9%F2%E8", info);
        info.Type = ST_SOCIAL;
        KS_TEST_URL("http://my.mail.ru/my/communities-search?q=winter&st=search&head=1&search=%CD%E0%E9%F2%E8", info);
        info.Name = SE_VKONTAKTE;
        KS_TEST_URL("http://vk.com/search?c%5Bq%5D=winter&c%5Bsection%5D=statuses", info);
        info.Query = "winter is coming";
        info.Name = SE_LIVEJOURNAL;
        KS_TEST_URL("http://www.livejournal.com/search/?q=winter+is+coming&area=posts", info);
        info.Name = SE_YA_RU;
        KS_TEST_URL("http://my.ya.ru/search_posts.xml?search_type=posts&text=winter+is+coming&nid1167535125313=", info);
        info.Name = SE_SPRASHIVAI_RU;
        KS_TEST_URL("http://sprashivai.ru/search?q=winter+is+coming", info);
        info.Name = SE_TUMBLR;
        info.Query = "funny cats";
        KS_TEST_URL("http://www.tumblr.com/tagged/funny+cats", info);
        info.Name = SE_VKONTAKTE;
        info.Flags = ESearchFlags(SF_SOCIAL | SF_SEARCH | SF_LOCAL | SF_MOBILE);
        KS_TEST_URL("http://m.vk.com/feed?section=search&q=funny%20cats", info);
        info.Name = SE_MOIKRUG;
        info.Query = "ivan";
        KS_TEST_URL("http://m.moikrug.yandex.ru/search/?keywords=ivan&submitted=1", info);
        info.Name = SE_ODNOKLASSNIKI;
        info.Query = "funny cats";
        info.Flags = ESearchFlags(SF_SOCIAL | SF_SEARCH | SF_LOCAL | SF_MOBILE);
        KS_TEST_URL("http://m.odnoklassniki.ru/dk?st.cmd=userAllSearch&st.search=funny+cats&_prevCmd=userMain&tkn=8186", info);
    }

    // music
    {
        TInfo info(SE_YANDEX, ST_MUSIC, "adele", ESearchFlags(SF_LOCAL | SF_SEARCH));

        KS_TEST_URL("http://music.yandex.ru/#!/search?text=adele", info);
        KS_TEST_URL("https://music.yandex.ru/search?text=adele", info);
        info.Name = SE_ITUNES;
        KS_TEST_URL("http://ax.search.itunes.apple.com/WebObjects/MZSearch.woa/wa/search?media=all&term=adele", info);
        info.Name = SE_ZVOOQ;
        KS_TEST_URL("http://zvooq.ru/#/search/?query=adele", info);
        info.Name = SE_GROOVESHARK;
        KS_TEST_URL("http://grooveshark.com/#!/search?q=adele", info);
        info.Name = SE_LAST_FM;
        KS_TEST_URL("http://www.lastfm.ru/search?q=adele&from=ac", info);
        info.Name = SE_PROSTOPLEER;
        KS_TEST_URL("http://prostopleer.com/search?q=adele", info);
        info.Name = SE_SOUNDCLOUD;
        KS_TEST_URL("http://soundcloud.com/search?q%5Bfulltext%5D=adele", info);
        info.Name = SE_MUZEBRA;
        KS_TEST_URL("http://muzebra.com/search/?q=adele", info);
        info.Name = SE_WEBORAMA;
        KS_TEST_URL("http://www.weborama.fm/#/music/found/adele/", info);
        info.Name = SE_101_RU;
        KS_TEST_URL("http://101.ru/?an=search_full&search=adele", info);
        info.Name = SE_PANDORA;
        KS_TEST_URL("http://www.pandora.com/search/adele", info);
        KS_TEST_URL("http://www.pandora.com/search/adele/song", info);
        KS_TEST_URL("http://www.pandora.com/search/adele/station", info);
        info.Name = SE_DEEZER;
        KS_TEST_URL("http://www.deezer.com/ru/search/adele", info);
    }
}
