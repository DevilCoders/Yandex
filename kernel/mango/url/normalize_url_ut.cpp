#include <kernel/mango/url/normalize_url.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TNormalizeUrlTest) {
    void Check(const TString &originalUrl, const TString &expectedUrl) {
        UNIT_ASSERT_VALUES_EQUAL(NMango::NormalizeUrl(originalUrl), expectedUrl);
        UNIT_ASSERT_VALUES_EQUAL(NMango::NormalizeUrl(NMango::NormalizeUrl(originalUrl)), NMango::NormalizeUrl(originalUrl));
    }

    Y_UNIT_TEST(TestStrip) {
        Check(" http://vkontakte.ru/wall-4439833_3196?hash=3476cf58890834f4be   ", "http://vkontakte.ru/wall-4439833_3196?hash=3476cf58890834f4be");
    }

    Y_UNIT_TEST(TestGates) {
        Check("http://vkontakte.ru/away.php?to=https%3A%2F%2Fspreadsheets.google.com%2Fspreadsheet%2Fviewform%3Fformkey%3DdGlIdnMza2Z0M3U2ekZjMzhuUVF4Z1E6MQ", "https://spreadsheets.google.com/spreadsheet/viewform?formkey=dGlIdnMza2Z0M3U2ekZjMzhuUVF4Z1E6MQ");
        Check("http://vk.com/away.php?to=https%3A%2F%2Fspreadsheets.google.com%2Fspreadsheet%2Fviewform%3Fformkey%3DdGlIdnMza2Z0M3U2ekZjMzhuUVF4Z1E6MQ", "https://spreadsheets.google.com/spreadsheet/viewform?formkey=dGlIdnMza2Z0M3U2ekZjMzhuUVF4Z1E6MQ");
        Check("http://t30p.ru/l.aspx?lenta.ru%2Fnews%2F2011%2F05%2F15%2Fshoot%2F", "http://lenta.ru/news/2011/05/15/shoot/");
        Check("http://lenta.ru/news/2011/05/15/shoot", "http://lenta.ru/news/2011/05/15/shoot");
        Check("http://www.headlines.ru/go.php?http://www.ria.ru/resume/20110729/409108959.html", "http://www.ria.ru/resume/20110729/409108959.html");
        Check("http://www.headlines.ru/go.php?http://%EB%EE%F1%EC%E8%EA%F0%EE%E1%E8%EE%F1.%F0%F4", "http://www.headlines.ru/go.php?http://%EB%EE%F1%EC%E8%EA%F0%EE%E1%E8%EE%F1.%F0%F4");
        Check("http://vkontakte.ru/away.php?to=http%3A%2F%2Fwww.dralzain.com%2FBook.aspx%3FSectionID%3D4%26RefID%3D17", "http://www.dralzain.com/Book.aspx?RefID=17&SectionID=4");
        Check("http://vk.com/away.php?to=http%3A%2F%2Fwww.youtube.com%2Fwatch%3Fv%3DNkGbTzyzPHo%26feature%3Dyoutu.be&h=01a0a537838c9ce540&post=41052_1098", "http://www.youtube.com/watch?v=NkGbTzyzPHo");
        Check("http://www.facebook.com/l.php?h=dAQFj_em_AQGDeItRNIkxWAK_e6krVvds2YJsGwweyfDERw&u=http%3A%2F%2Fgidepark.ru%2Fcommunity%2F129%2Farticle%2F475265", "http://gidepark.ru/community/129/article/475265");
    }

    Y_UNIT_TEST(TestGoogleAnalytics) {
        Check("http://news.rambler.ru/7430740/?utm_source=smi2&utm_medium=tiser&utm_campaign=obmenka", "http://news.rambler.ru/7430740/");
        Check("http://somerandomsite.ru/gorss.php?http://www.sports.ru/football/112245982.html&utm_source=twitterfeed&utm_medium=twitter", "http://somerandomsite.ru/gorss.php?http://www.sports.ru/football/112245982.html");
    }

    Y_UNIT_TEST(TestTwitter) {
        Check("https://twitter.com/?_escaped_fragment_=/unicodemonkey/statuses/83647388673257473", "https://twitter.com/unicodemonkey/statuses/83647388673257473");
        Check("https://twitter.com/?_escaped_fragment_=whatever", "https://twitter.com/?_escaped_fragment_=whatever");
        Check("http://twitter.com/?_escaped_fragment_=/sai_tools%3Futm_source%3Dvertical%26utm_medium%3Darticlebottom%26utm_term%3D%26utm_content%3Dtwitter%26utm_campaign%3Drecirc", "http://twitter.com/sai_tools");
    }

    Y_UNIT_TEST(TestLivejournal) {
        Check("http://lj-russian.livejournal.com/179127.html?format=light&nc=23", "http://lj-russian.livejournal.com/179127.html");
        Check("http://lj-russian.livejournal.com/179127.html?thread=1135543&format=light", "http://lj-russian.livejournal.com/179127.html?thread=1135543");
        Check("http://www.livejournal.com/shop/vgift.bml?cat=featured", "http://www.livejournal.com/shop/vgift.bml?cat=featured");
    }

    Y_UNIT_TEST(TestYoutube) {
        Check("http://www.youtube.com/watch?v=1CYB4-X0DNs&feature=related", "http://www.youtube.com/watch?v=1CYB4-X0DNs");
        Check("http://youtube.com/watch?v=O7mvIrPMctY&feature=youtu.be&a", "http://www.youtube.com/watch?v=O7mvIrPMctY");
        Check("http://www.youtube.com/das_captcha?next=/watch%3Fv%3DUAe7tVKBW5U%26feature%3Dyoutu.be%26a", "http://www.youtube.com/watch?v=UAe7tVKBW5U");
        Check("http://www.youtube.com/verify_controversy?next_url=/watch%3Fv%3DnunGLGRMeDA%26feature%3Dyoutu.be%26a", "http://www.youtube.com/watch?v=nunGLGRMeDA");
        Check("http://www.youtube.com/verify_age?next_url=/watch%3Fv%3D5HZQ7l2xYqA%26oref%3Dhttp%253A%252F%252Fwww.funnyjunk.com%252Ffunny_pictures%252F3188962%252FDescription%252F", "http://www.youtube.com/watch?v=5HZQ7l2xYqA");
        Check("http://m.youtube.com/verify_age?client=mv-google&gl=RU&hl=ru&next=/watch%3Fv%3Ds6b33PTbGxk", "http://www.youtube.com/watch?v=s6b33PTbGxk");
        Check("http://m.youtube.com/watch?v=mWJG-sB7H4Y", "http://www.youtube.com/watch?v=mWJG-sB7H4Y");
        Check("http://m.youtube.com/#/watch?bmb=1&desktop_uri=http%3A%2F%2Fwww.youtube.com%2Fwatch%3Fv%3DLk3ibIGKTYA&gl=US&v=Lk3ibIGKTYA", "http://www.youtube.com/watch?v=Lk3ibIGKTYA");
        Check("http://m.youtube.com/#/watch?desktop_uri=%2Fwatch%3Ffeature%3Dplayer_embedded%26v%3DsNbpfNkzjZ4&feature=player_embedded&v=sNbpfNkzjZ4&gl=RU", "http://www.youtube.com/watch?v=sNbpfNkzjZ4");
        Check("http://www.youtube.com/watch?v=rOP8gZMDnKc", "http://www.youtube.com/watch?v=rOP8gZMDnKc");
        Check("http://youtu.be/rOP8gZMDnKc", "http://www.youtube.com/watch?v=rOP8gZMDnKc");
        Check("http://youtu.be/", "http://youtu.be/");
    }

    Y_UNIT_TEST(TestCutTrailingGarbage) {
        Check("http://t.co/SRJ4ULS!", "http://t.co/SRJ4ULS");
        Check("http://t.co/qxpvbaB:", "http://t.co/qxpvbaB");
        Check("http://t.co/m9dN3M9),", "http://t.co/m9dN3M9");
        Check("http://t.co/uQ1zYWU%D1%80%D0%BE%D1%81%D1%81%D0%B8%D0%B9%D1%81%D0%BA%D0%B8%D0%B9-%D0%B3%D0%B5%D1%80%D0%B1%D0%B0%D1%80%D0%B8%D0%B9-%D1%84%D0%B8%D0%BB%D1%8C%D0%BC-%D0%BF%D0%B5%D1%80%D0%B2%D1%8B%D0%B9-%D0%B8%D1%83%D0%B4%D0%B8%D0%BD%D0%BE-%D0%B4%D0%B5%D1%80%D0%B5%D0%B2%D0%BE", "http://t.co/uQ1zYWU");
        Check("http://goo.gl/fb/0OiGB", "http://goo.gl/fb/0OiGB");
        Check("http://clck.ru/N/OgnL", "http://clck.ru/N/OgnL");
        Check("http://feeds.feedburner.com/~r/com/restudio-ua/~3/Nb7P3-zeclc/fisi?utm_source=feedburner&utm_medium=twitter&utm_campaign=restudio_adv", "http://feeds.feedburner.com/~r/com/restudio-ua/~3/Nb7P3-zeclc/fisi");
    }

    Y_UNIT_TEST(TestCgiParamsSort) {
        Check("http://www.realestatepress.es/MostrarNoticia.asp?M=0&Id=14278", "http://www.realestatepress.es/MostrarNoticia.asp?Id=14278&M=0");
        Check("https://specto.yandex.ru/betula00/debug?info=table&table=mango/data/old_mango_bases/1322122308/resources_downloaded&start=64&count=64", "https://specto.yandex.ru/betula00/debug?count=64&info=table&start=64&table=mango/data/old_mango_bases/1322122308/resources_downloaded");
    }

    Y_UNIT_TEST(TestSlashes) {
        Check("http://ban.jo/invite/tw1/", "http://ban.jo/invite/tw1/");
        Check("http://www.bbc.co.uk/russian/topics/israel/", "http://www.bbc.co.uk/russian/topics/israel/");
        Check("http://adne.info/", "http://adne.info/");
        Check("http://headpinz.ru/2011/11/19/%d0%bd%d0%be%d0%b2%d0%b0%d1%8f-%d1%80%d0%b5%d0%ba%d0%bb%d0%b0%d0%bc%d0%b0-fedex/", "http://headpinz.ru/2011/11/19/%D0%BD%D0%BE%D0%B2%D0%B0%D1%8F-%D1%80%D0%B5%D0%BA%D0%BB%D0%B0%D0%BC%D0%B0-fedex/");
    }
    Y_UNIT_TEST(TestRemoveFragment) {
        Check("http://www.youtube.com/watch?v=xVxvwL2SgLQ#", "http://www.youtube.com/watch?v=xVxvwL2SgLQ");
        Check("http://www.youtube.com/watch?v=xVxvwL2SgLQ#some_fragment", "http://www.youtube.com/watch?v=xVxvwL2SgLQ");
        Check("http://yandex.ru/#do_search=5", "http://yandex.ru/");
    }
    Y_UNIT_TEST(TestHtEntDecode) {
        Check("http://www.last.fm/music/Cory+Enemy+&amp;+Dillon+Francis", "http://www.last.fm/music/Cory+Enemy+&+Dillon+Francis");
        Check("http://onlinepc.ch/index.cfm?CFID=2033764&amp;CFTOKEN=59367857&amp;artikel_id=33205&amp;page=104029", "http://onlinepc.ch/index.cfm?CFID=2033764&CFTOKEN=59367857&artikel_id=33205&page=104029");
        Check("http://www.gurselkorat.blogspot.com/search/label/%C4%B0skender%20Pala&apos;ya%20Reddiye", "http://www.gurselkorat.blogspot.com/search/label/%C4%B0skender%20Pala'ya%20Reddiye");
    }
}

