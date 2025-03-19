#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc64() {
    // Mail_2
    {
        TInfo info(SE_YANDEX, ST_WEB, "winter", ESearchFlags(SF_LOCAL | SF_SEARCH | SF_MAIL));
        info.Name = SE_YANDEX;
        KS_TEST_URL("https://mail.yandex.ru/neo2/#search/request=winter", info);
        info.Name = SE_MAIL;
        KS_TEST_URL("https://e.mail.ru/cgi-bin/msglist?back=1#/search/?search=%D0%9D%D0%B0%D0%B9%D1%82%D0%B8&q_query=winter&st=search&from_suggest=0&from_search=0", info);
        info.Name = SE_GOOGLE;
        KS_TEST_URL("https://mail.google.com/mail/u/1/?shva=1#search/winter", info);
        info.Name = SE_QIP;
        KS_TEST_URL("https://mail.qip.ru/search/Inbox;/?search=winter", info);
        info.Name = SE_RAMBLER;
        KS_TEST_URL("https://mail.rambler.ru/#/folder/INBOX/search=winter/", info);
        info.Name = SE_I_UA;
        KS_TEST_URL("http://mbox2.i.ua/list/INBOX/?text=winter#", info);
        info.Name = SE_BIGMIR;
        KS_TEST_URL("http://mbox.bigmir.net/list/drafts/?text=winter#", info);
        info.Name = SE_GO_KM_RU;
        KS_TEST_URL("http://mail.km.ru/folder/messagesearch.htm?_from=&_to=&_text=&_subject=winter&folder=INBOX&_minattachsize=&_maxattachsize=&command_msearch=%C8%F1%EA%E0%F2%FC", info);
    }
}
