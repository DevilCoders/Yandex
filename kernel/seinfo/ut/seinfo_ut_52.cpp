#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc52() {
    {
        TInfo info(SE_ODNOKLASSNIKI, ST_WEB, "", ESearchFlags(SF_SOCIAL));
        KS_TEST_URL("http://odnoklassniki.ru/profile/557970262236/friends", info);
        info.Name = SE_FACEBOOK;
        KS_TEST_URL("https://www.facebook.com/find-friends/browser/", info);
        info.Name = SE_LINKEDIN;
        KS_TEST_URL("http://www.linkedin.com/nhome/nux?ph2=true&trk=hb_tab_home_top", info);
        info.Name = SE_MYSPACE;
        KS_TEST_URL("https://myspace.com/discover/trending", info);
        info.Name = SE_MAIL;
        info.Flags = SF_SOCIAL;
        KS_TEST_URL("http://my.mail.ru/bk/example#page=/friends?", info);
        info.Name = SE_VKONTAKTE;
        KS_TEST_URL("http://vk.com/feed", info);
        KS_TEST_URL("http://cs11000.vk.me/u69283593/a_671eaa98.jpg", info);
        info.Name = SE_LIVEJOURNAL;
        KS_TEST_URL("http://example.livejournal.com/friend", info);
        info.Name = SE_YA_RU;
        KS_TEST_URL("http://example.ya.ru/index_opinions.xm", info);
        info.Name = SE_MAMBA;
        KS_TEST_URL("http://www.mamba.ru/", info);
    }
}
