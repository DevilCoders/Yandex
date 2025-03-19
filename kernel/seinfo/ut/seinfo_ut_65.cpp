#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc65() {
    {
        TInfo info(SE_YANDEX, ST_WEB, "", ESearchFlags(SF_MAIL));
        KS_TEST_URL("https://mail.yandex.ru/neo2/#sent", info);
        KS_TEST_URL("https://mail.yandex.ru/lite/inbox", info);
        info.Name = SE_QIP;
        KS_TEST_URL("https://mail.qip.ru/view/Inbox~1", info);
        info.Name = SE_GO_KM_RU;
        KS_TEST_URL("http://mail.km.ru/folder/folder.htm?folder=INBOX&sort=date", info);
        info.Name = SE_GO_KM_RU;
        KS_TEST_URL("http://mail.km.ru/message/message.htm?folder=INBOX&message=00000000-FFFFFFFF-51DF899F-000001DB&sort=date", info);
        info.Name = SE_RAMBLER;
        KS_TEST_URL("https://mail.rambler.ru/#/folder/DraftBox", info);
        info.Name = SE_MAIL;
        KS_TEST_URL("https://e.mail.ru/cgi-bin/msglist?back=1#/message/13705218440000000571", info);
        info.Name = SE_GOOGLE;
        KS_TEST_URL("https://mail.google.com/mail/?shva=1#inbox/13fd0734a6a31a2", info);
        info.Name = SE_YAHOO;
        KS_TEST_URL("http://ru-mg42.mail.yahoo.com/neo/launch?.rand=c15o8cj7djqis#mail", info);
        info.Name = SE_LIVE_COM;
        KS_TEST_URL("https://snt146.mail.live.com/mail/InboxLight.aspx?n=571829493#n=1185997575&fid=4", info);
        info.Name = SE_BIGMIR;
        KS_TEST_URL("http://mbox.bigmir.net/read/INBOX/51dff32eebdc/?_rand=163131456", info);
        info.Name = SE_I_UA;
        KS_TEST_URL("http://mbox2.i.ua/read/INBOX/51dff424ae94/?_rand=1631589597", info);
        info.Name = SE_NGS_RU;
        KS_TEST_URL("https://mail.ngs.ru/Session/1993582-u1AZgnYbyCFUN6wpeeTq-jizcflv/Message.wssp?Mailbox=INBOX&MSG=1", info);
    }
}
