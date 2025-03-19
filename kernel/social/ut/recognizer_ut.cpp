#include <kernel/social/recognizer.h>

#include <library/cpp/testing/unittest/registar.h>

class TIdentityRecognizerTest: public TTestBase {
private:
    UNIT_TEST_SUITE(TIdentityRecognizerTest)
        UNIT_TEST(TestYaRu);
        UNIT_TEST(TestFacebook);
        UNIT_TEST(TestTwitter);
        UNIT_TEST(TestVKontakte);
        UNIT_TEST(TestOdnoklassniki);
        UNIT_TEST(TestLiveJournal);
        UNIT_TEST(TestMoiMirMailRu);
        UNIT_TEST(TestLinkedIn);
        UNIT_TEST(TestMoiKrug);
        UNIT_TEST(TestGooglePlus);
        UNIT_TEST(TestLiveInternet);
        UNIT_TEST(TestFreeLance);
        UNIT_TEST(TestFriendFeed);
        UNIT_TEST(TestMirTesen);
        UNIT_TEST(TestFoursquare);
        UNIT_TEST(TestInstagram);
    UNIT_TEST_SUITE_END();

    void TestValidUrl(const TString& url, const TString& identity, NSocial::ESocialNetwork network, NSocial::EPageParser parser);
    void TestInvalidUrl(const TString& url);

public:
    void TestYaRu();
    void TestFacebook();
    void TestTwitter();
    void TestVKontakte();
    void TestOdnoklassniki();
    void TestLiveJournal();
    void TestMoiMirMailRu();
    void TestLinkedIn();
    void TestMoiKrug();
    void TestGooglePlus();
    void TestLiveInternet();
    void TestFreeLance();
    void TestFriendFeed();
    void TestMirTesen();
    void TestFoursquare();
    void TestInstagram();
};

UNIT_TEST_SUITE_REGISTRATION(TIdentityRecognizerTest);

void TIdentityRecognizerTest::TestValidUrl(const TString& url,
                                           const TString& identity,
                                           NSocial::ESocialNetwork network,
                                           NSocial::EPageParser parser) {
    NSocial::TIdentityPair pair;
    if (!NSocial::ExtractIdent(url, pair)) {
        UNIT_FAIL("Parse error at url " + url);
    } else if (pair.Network != network) {
        UNIT_FAIL("Incorrect network for url [" + url + "]. [" + ToString(network) + "] expected, but [" + ToString(pair.Network) + "] found");
    } else if (pair.Id != identity) {
        UNIT_FAIL("Incorrect id for url [" + url + "]. [" + identity + "] expected, but [" + pair.Id + "] found");
    } else if (pair.Parser != parser) {
        UNIT_FAIL("Incorrect parser for url [" + url + "]. [" + ToString((int)parser) + "] expected, but [" + ToString((int)pair.Parser) + "] found");
    }
}

void TIdentityRecognizerTest::TestInvalidUrl(const TString& url) {
    NSocial::TIdentityPair pair;
    if (ExtractIdent(url, pair)) {
        UNIT_FAIL("Url [" + url + "] successfully parsed, but should not.");
    }
}

#define TestValidUrlWithNetwork(URL, ID, PARSER) TestValidUrl(URL, ID, network, PARSER)
#define DEFINE_NETWORK(N) NSocial::ESocialNetwork network = NSocial::N;

void TIdentityRecognizerTest::TestYaRu() {
    DEFINE_NETWORK(YaRu);
    TestInvalidUrl("http://kunenok.ya.ru/search_users_by_interest.xml?name=birthday&text=31/5");
    TestValidUrlWithNetwork("http://SemyonBaryba.ya.ru/foaf.xml", "semyonbaryba", NSocial::PP_YARU_FOAF);
    TestInvalidUrl("http://botay.ya.ru/blabla");
    TestInvalidUrl("http://kunenok.ya.ru/index_video.xml");
    TestValidUrlWithNetwork("http://vizaric.ya.ru/#y5__id42", "vizaric", NSocial::PP_NONE);
    TestValidUrlWithNetwork("http://Dark_lady.ya.ru", "dark-lady", NSocial::PP_NONE);
    TestInvalidUrl("http://ann-babich.ya.ru/index_fotki.xml");
    TestInvalidUrl("http://lily-vagary.ya.ru/index_fotki.xml#y5__id22");
    TestInvalidUrl("http://dasha151515.ya.ru/posts.xml?ncrnd=75");
    TestInvalidUrl("http://surfman2011.ya.ru/rss/posts.xml");

}

void TIdentityRecognizerTest::TestFacebook() {
    DEFINE_NETWORK(Facebook);
    TestInvalidUrl("http://www.facebook.com/profile.php?id=94700512&ref=ts#o2ang");
    TestInvalidUrl("http://www.facebook.com/profile.php?id=100002568171810#!/groups/135911056489938");
    TestInvalidUrl("http://ru-ru.facebook.com/l.php?u=http%3A%2F%2Fwww.yandex.ru&h=VAQGMAT0D&s=1");
    TestInvalidUrl("http://facebook.com/profile.php?id=100000789139672&ref=ts#!/aaponcio1");
    TestInvalidUrl("http://www.facebook.com/profile.php?id=100001037001405&ref=ts#!/profile.php%3Fid=100000088338366");
    TestInvalidUrl("http://facebook.com/events/216847995054082");
    TestInvalidUrl("http://facebook.com/r.php");
    TestValidUrlWithNetwork("http://tr-tr.facebook.com/pages/Sevil-MEMETOVA/302657302960", "302657302960", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://facebook.com/pages/Star-Cruises-Singapore/190575967623819", "190575967623819", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://www.facebook.com/pages/%D0%9A%D0%BB%D0%B8%D0%BD%D0%B8%D0%BA%D0%B0-%D1%81%D1%82%D0%BE%D0%BC%D0%B0%D1%82%D0%BE%D0%BB%D0%BE%D0%B3%D0%B8%D0%B8-%D0%9C%D0%95%D0%94%D0%98%D0%90%D0%9D/117840144996033", "117840144996033", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://www.facebook.com/pages/foo/117840144996033", "117840144996033", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://www.facebook.com/pages/117840144996033/117840144996033", "117840144996033", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://www.facebook.com/pages/11784014499603/117840144996033", "117840144996033", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://www.facebook.com/pages/117840144996033/11784014499603", "11784014499603", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://www.facebook.com/pages/11784014499603/11784014499603", "11784014499603", NSocial::PP_FACEBOOK);
    TestInvalidUrl("http://facebook.com/p.php?id=21715534&l=8ae407c52e");
    TestValidUrlWithNetwork("http://kU-tr.facebook.com/people/Ahmed-Rania-Ranya/100001621705166", "100001621705166", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://www.facebook.com/ilya.Segalovich", "ilya.segalovich", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://www.facebook.com/profile.php?id=100002298210217", "100002298210217", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://tr-tr.facebook.com/pages/Sevil-MEMETOVA/302657302960", "302657302960", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://www.facebook.com/100000595119793", "100000595119793", NSocial::PP_FACEBOOK);
    TestInvalidUrl("http://facebook.com/home.php");
    TestInvalidUrl("http://www.facebook.com/me");
    TestInvalidUrl("http://www.facebook.com/notes");
    TestValidUrlWithNetwork("http://ru-ru.facebook.com/people/Katherine-Vorob\'eva/100002790710404", "100002790710404", NSocial::PP_FACEBOOK);
    TestInvalidUrl("https://www.facebook.com/jesperkyd/whatever");
    TestInvalidUrl("http://developers.facebook.com/plugins/?footer=1");
    TestInvalidUrl("http://www.facebook.com/sharer.php?u=http://www.pokazuha.ru/view/topic.cfm?key_or=802328");
    TestValidUrlWithNetwork("http://facebook.com/pages/Star-Cruises-Singapore/190575967623819?sk=info", "190575967623819", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://tr-tr.facebook.com/pages/AZRA-AKIN/185455871485152", "185455871485152", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://tr-tr.facebook.com/17254724299?sk=info", "17254724299", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://tr-tr.facebook.com/pages/Nasuh-Mahruki/145360958846875?sk=info", "145360958846875", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://tr-tr.facebook.com/Official.Fazil.Say/info", "official.fazil.say", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://www.facebook.com/pages/Ara-G%C3%BCler/108682422485575?sk=info", "108682422485575", NSocial::PP_FACEBOOK);
    TestValidUrlWithNetwork("http://facebook.com/profile.php?id=21715534&sk=info", "21715534", NSocial::PP_FACEBOOK);
    TestInvalidUrl("http://facebook.com/profile.php?id=21715534/info");
    TestInvalidUrl("http://www.facebook.com/pages/%D0%9D%D0%B8%D1%81%D0%BE%D0%BD-%D0%9B%D0%B8%D0%B0%D0%BC/175379769184284?rf=108018409218583");
}

void TIdentityRecognizerTest::TestTwitter() {
    DEFINE_NETWORK(Twitter);
    TestInvalidUrl("http://twitter.com/&4S");
    TestValidUrlWithNetwork("http://twitter.com/#!/CassieBrittain", "cassiebrittain", NSocial::PP_TWITTER);
    TestInvalidUrl("http://twitter.com/!#/CassieBrittain");
    TestValidUrlWithNetwork("http://twitter.com/NostalGeese", "nostalgeese", NSocial::PP_TWITTER);
    TestValidUrlWithNetwork("http://twitter.com/AlexeevaDanya#/AlexeevaDanya", "alexeevadanya", NSocial::PP_TWITTER);
    TestInvalidUrl("http://twitter.com/HAI_KU/?haiku");
    TestInvalidUrl("http://twitter.com/##/varell_JR");
    TestInvalidUrl("http://twitter.com/#%21%2F1f_jef");
    TestInvalidUrl("http://twitter.com/#?#/tyahikmah");
    TestInvalidUrl("http://twitter.com/%23!/1Dsweethearts");
    TestValidUrlWithNetwork("http://twitter.com/Maru2day/#favorites", "maru2day", NSocial::PP_TWITTER);
    TestInvalidUrl("http://twitter.com/Mitzy&NadaMas");
    TestInvalidUrl("http://www.twitter.com/RuslanMuydinov\"><img");
    TestValidUrlWithNetwork("http://mobile.twitter.com/raflata_4ever/#raflata_4ever", "raflata_4ever", NSocial::PP_TWITTER);
}

void TIdentityRecognizerTest::TestVKontakte() {
    DEFINE_NETWORK(VKontakte);
    TestValidUrlWithNetwork("http://vkontakte.ru/1", "id1", NSocial::PP_VK);
    TestInvalidUrl("http://vkontakte.ru/club14790147");
    TestInvalidUrl("http://vkontakte.ru/id");
    TestInvalidUrl("http://vkontakte.ru/xyz.aBc?1234");
    TestValidUrlWithNetwork("http://vk.com/id0000000064094", "id64094", NSocial::PP_VK);
    TestInvalidUrl("http://vk.com/id1/");
    TestInvalidUrl("http://vkontakte.ru/groups.php?99200");
    TestInvalidUrl("http://vk.com/id1#!/id2");
    TestInvalidUrl("http://vk.com/wall1234");
    TestInvalidUrl("http://vkontakte.ru/note31292014_9865594");
    TestInvalidUrl("http://vk.com/albums1234");
    TestInvalidUrl("http://vk.com/photos1234");
    TestInvalidUrl("http://vk.com/wall1234");
    TestInvalidUrl("http://vk.com/event9782420");
    TestValidUrlWithNetwork("http://VKONTAKTE.RU/ID104361835", "id104361835", NSocial::PP_VK);
    TestInvalidUrl("http://vk.com/Karim/1999-2012");
    TestInvalidUrl("http://vk.com/editProfil/php/act");
    TestInvalidUrl("http://vk.com/feed#/album3969103_122335713");
    TestValidUrlWithNetwork("http://vk.com/id68202727_68202727", "id68202727_68202727", NSocial::PP_VK);
    TestInvalidUrl("http://vk.com/id67718849\">");
    TestValidUrlWithNetwork("http://vk.com/ru_rus", "ru_rus", NSocial::PP_VK);
    TestInvalidUrl("http://vk.com/wall115377730?own=1");
    TestInvalidUrl("http://vk.com/videos50224665");
    TestValidUrlWithNetwork("http://VkOnTaKtE.rU/iD84584549", "id84584549", NSocial::PP_VK);
    TestInvalidUrl("http://vkontakte.ru/ARTEM_DERESCH/id88394220");
    TestValidUrlWithNetwork("http://vkontakte.ru/ASCII_ART_APP", "ascii_art_app", NSocial::PP_VK);
    TestInvalidUrl("http://vkontakte.ru/app/id70087276");
    TestInvalidUrl("http://vkontakte.ru/app1713637#/profile/1766818");
    TestInvalidUrl("http://vkontakte.ru/app1850211#/article/3935523");
    TestInvalidUrl("http://vkontakte.ru/club");
    TestInvalidUrl("http://vkontakte.ru/club23841166....vkontakte.ru/public24655251");
    TestInvalidUrl("http://vkontakte.ru/edit321543");
    TestInvalidUrl("http://vkontakte.ru/extremeclown\">%D0%AF");
    TestInvalidUrl("http://vkontakte.ru/flirtomaniya?ref=2#!id53415946");
    TestValidUrlWithNetwork("http://vkontakte.ru/fotometki#/users/928980", "fotometki", NSocial::PP_VK);
    TestInvalidUrl("http://vkontakte.ru/id102188106\">%D0%AF");
    TestValidUrlWithNetwork("http://vkontakte.ru/id139591602_13", "id139591602_13", NSocial::PP_VK);
    TestInvalidUrl("http://vkontakte.ru/id40731384\">");
    TestInvalidUrl("http://vkontakte.ru/polinaas%23/polinaas");
    TestInvalidUrl("http://vkontakte.ru/public.php25711731");
    TestInvalidUrl("http://vkontakte.ru/videos119777155?section=album_36762891");
    TestInvalidUrl("http://vkontakte.ru/d18850626&41638");
    TestValidUrlWithNetwork("http://vkontakte.ru/regstep3#/edit?act=contacts#/id137452515", "regstep3", NSocial::PP_VK);
    TestInvalidUrl("http://cs5457.vkontakte.ru/u37229897/129600765/x_3f1aa8b4.jpg");
    TestValidUrlWithNetwork("http://vkontakte.ru/foaf.php?id=1", "id1", NSocial::PP_VK_FOAF);
    TestValidUrlWithNetwork("http://vk.com/foaf.php?id=123", "id123", NSocial::PP_VK_FOAF);
    TestInvalidUrl("http://vk.com/foaf.php?id=abc");
    TestInvalidUrl("http://vk.com/gifts123");
    TestInvalidUrl("http://vk.com/");
    TestInvalidUrl("http://vk.com/login.php?act=slogin&role=fast&no_redirect=1&to=&s=0");
    TestInvalidUrl("https://login.vk.com/?role=fast&_origin=http://vk.com&ip_h=a78877b68642ea6f32&to=Z2lmdHMxMzg0Nzc5MzY-");
    TestValidUrlWithNetwork("http://vk.com/feed_me", "feed_me", NSocial::PP_VK);
    TestValidUrlWithNetwork("http://vk.com/club_666", "club_666", NSocial::PP_VK);
    TestValidUrlWithNetwork("http://vkontakte.ru/public.php_photosk", "public.php_photosk", NSocial::PP_VK);
    TestValidUrlWithNetwork("http://vk.com/public.php_sink", "public.php_sink", NSocial::PP_VK);
}

void TIdentityRecognizerTest::TestOdnoklassniki() {
    DEFINE_NETWORK(Odnoklassniki);
    TestValidUrlWithNetwork("http://www.odnoklassniki.ru/profile270323262020", "profile270323262020", NSocial::PP_ODNOKLASSNIKI);
    TestInvalidUrl("http://odnoklassniki.ru/s@ne4ka");
    TestInvalidUrl("http://odnoklassniki.ru/#/profile/530780607514");
    TestInvalidUrl("http://odnoklassniki.ru/#/anna.ershova");
    TestValidUrlWithNetwork("http://odnoklassniki.ru/user/143322124", "profile/143322124", NSocial::PP_ODNOKLASSNIKI);
    TestValidUrlWithNetwork("http://www.odnoklassniki.ru/profile/131206037158", "profile/131206037158", NSocial::PP_ODNOKLASSNIKI);
    TestValidUrlWithNetwork("http://ok.ru/profile/131206037158", "profile/131206037158", NSocial::PP_ODNOKLASSNIKI);
    TestValidUrlWithNetwork("http://www.odnoklassniki.ru/profile131206037158", "profile131206037158", NSocial::PP_ODNOKLASSNIKI);
    TestValidUrlWithNetwork("http://www.odnoklassniki.ru/o_0pepsi", "o_0pepsi", NSocial::PP_ODNOKLASSNIKI);
    TestValidUrlWithNetwork("http://www.ok.ru/o_0pepsi", "o_0pepsi", NSocial::PP_ODNOKLASSNIKI);
    TestInvalidUrl("http://odnoklassniki.ru/alexander.perendzhiyev?st.cmd=userMain");
    TestInvalidUrl("http://odnoklassniki.ru/sidite=tolko=tut");
    TestInvalidUrl("http://www.odnoklassniki.ru/#/profile/335481989178/photos");
    TestValidUrlWithNetwork("http://www.odnoklassniki.ru/profile/513587330701#/profile/509615222089", "profile/513587330701", NSocial::PP_ODNOKLASSNIKI);
    TestValidUrlWithNetwork("http://odnoklassniki.ru/profile/347876139064#/profile/520702130860", "profile/347876139064", NSocial::PP_ODNOKLASSNIKI);
    TestInvalidUrl("http://www.odnoklassniki.ru/game/magicballs#/profile/514036511188");
    TestInvalidUrl("http://www.odnoklassniki.ru/profile/347787117080?st.friendId=aoymnibqezwcczrpm0qcjguofdrbawavsapwd");
    TestInvalidUrl("http://www.odnoklassniki.ru/games/#/guests");
    TestInvalidUrl("http://www.odnoklassniki.ru/uzer/102558280572");
}

void TIdentityRecognizerTest::TestLiveJournal() {
    DEFINE_NETWORK(LiveJournal);
    TestInvalidUrl("http://community.livejournal.com/moroQue_ua");
    TestValidUrlWithNetwork("http://users.livejournal.com/_hate2Loveu_", "-hate2loveu-", NSocial::PP_NONE);
    TestValidUrlWithNetwork("http://users.livejournal.com/_hate2Loveu_/profile", "-hate2loveu-", NSocial::PP_LIVEJOURNAL);
    TestValidUrlWithNetwork("http://users.livejournal.com/_hate2Loveu_/info", "-hate2loveu-", NSocial::PP_LIVEJOURNAL);
    TestValidUrlWithNetwork("http://anton_nossik.livejournal.com", "anton-nossik", NSocial::PP_LIVEJOURNAL);
    TestValidUrlWithNetwork("http://anton_nossik.livejournal.com/", "anton-nossik", NSocial::PP_LIVEJOURNAL);
    TestValidUrlWithNetwork("http://www.livejournal.com/userinfo.bml?user=doLboeb", "dolboeb", NSocial::PP_LIVEJOURNAL);
    TestInvalidUrl("http://livejournal.com/users/add.bml");
    TestInvalidUrl("http://www.livejournal.com/fireheart251");
    TestInvalidUrl("http://www.livejournal.com/friends/add.bml?user=yummehz_kaboomm");
    TestInvalidUrl("http://www.livejournal.com/tools/memories.bml?user=yuna177");
    TestInvalidUrl("http://lia-zach.livejournal.com.livejournal.com");
    TestInvalidUrl("http://www.livejournal.com/friends/add.bml?user=_omgjacki");
    TestInvalidUrl("http://www.livejournal.com/userinfo.bml?user=harry_temporis&mode=full");
    TestInvalidUrl("http://www.livejournal.com/users/harrypotterrpg.htm");
    TestInvalidUrl("http://www.livejournal.com/inbox/compose.bml?user=loki_sert");
    TestInvalidUrl("http://www.livejournal.com/tools/friendlist.bml?user=trash_glamor");
    TestInvalidUrl("http://www.livejournal.com/~arrete/friends/add.bml?user=arrete");
    TestInvalidUrl("http://www.livejournal.com/todo/index.bml?user=mewsician");
    TestInvalidUrl("http://www.livejournal.com/users/community/gabriel_daily");
    TestInvalidUrl("http://navalny.livejournal.com/profile?socconns=pfriends&comms=cfriends");
    TestValidUrlWithNetwork("http://livejournal.com/users/harrypotterrpg/profile/", "harrypotterrpg", NSocial::PP_LIVEJOURNAL);
    TestValidUrlWithNetwork("http://abc.livejournal.com/profile/", "abc", NSocial::PP_LIVEJOURNAL);
    TestValidUrlWithNetwork("http://www.livejournal.com/profile?userid=60194458", "60194458", NSocial::PP_LIVEJOURNAL);
}

void TIdentityRecognizerTest::TestMoiMirMailRu() {
    DEFINE_NETWORK(MoiMirMailRu);
    TestValidUrlWithNetwork("http://my.mail.ru/mail/julia_m99/", "mail/julia_m99", NSocial::PP_NONE);
    TestInvalidUrl("http://blogs.mail.ru/mail/dolinkaschachan");
    TestValidUrlWithNetwork("http://my.mail.ru/mail/dolinkaschachan", "mail/dolinkaschachan", NSocial::PP_NONE);
    TestValidUrlWithNetwork("http://my.mail.ru/mail/koshka.dzhuliya/#page=/mail/azamat64.toyota", "mail/koshka.dzhuliya", NSocial::PP_NONE);
    TestValidUrlWithNetwork("http://my.mail.ru/inbox/arajkee/", "inbox/arajkee", NSocial::PP_NONE);
    TestInvalidUrl("http://my.mail.ru/arik.karapetyan");
    TestValidUrlWithNetwork("http://my.mail.ru/mailua/julia_m99/foaf", "mailua/julia_m99", NSocial::PP_MOIMIR);
    TestValidUrlWithNetwork("http://my.mail.ru/mail/julia_m99/foaf", "mail/julia_m99", NSocial::PP_MOIMIR);
    TestInvalidUrl("http://blogs.mail.ru/mail/dolinkaschachan/foaf");
    TestValidUrlWithNetwork("http://my.mail.ru/mail/dolinkaschachan/foaf", "mail/dolinkaschachan", NSocial::PP_MOIMIR);
    TestValidUrlWithNetwork("http://my.mail.ru/mail/koshka.dzhuliya/#page=/mail/azamat64.toyota", "mail/koshka.dzhuliya", NSocial::PP_NONE);
    TestValidUrlWithNetwork("http://my.mail.ru/inbox/arajkee/foaf", "inbox/arajkee", NSocial::PP_MOIMIR);
    TestValidUrlWithNetwork("http://my.mail.ru/arik.karapetyan/foaf", "arik.karapetyan/foaf", NSocial::PP_NONE);
    TestValidUrlWithNetwork("http://my.mail.ru/yandex.ru/clients", "yandex.ru/clients", NSocial::PP_NONE);
    TestValidUrlWithNetwork("http://my.mail.ru/yandex.ru/clients/foaf", "yandex.ru/clients", NSocial::PP_MOIMIR);
    TestInvalidUrl("http://my.mail.ru/my/search");
    TestInvalidUrl("http://my.mail.ru/community/search");
}

void TIdentityRecognizerTest::TestLinkedIn() {
    DEFINE_NETWORK(LinkedIn);
    TestInvalidUrl("http://linkedin.com/in/katherinefunk/page.html");
    TestValidUrlWithNetwork("http://www.linkedin.com/pub/subodH-das/a/128/142", "subodh-das/a/128/142", NSocial::PP_LINKEDIN);
    TestValidUrlWithNetwork("http://www.linkedin.com/pub/kyle-attarian-cfp%C2%AE-aif%C2%AE/18/571/b2a", "kyle-attarian-cfp%c2%ae-aif%c2%ae/18/571/b2a", NSocial::PP_LINKEDIN);
    TestValidUrlWithNetwork("http://www.linkedin.com/in/kyleattarian", "kyleattarian", NSocial::PP_LINKEDIN);
    TestValidUrlWithNetwork("http://en.linkedin.com/in/LeelaTuranga", "leelaturanga", NSocial::PP_LINKEDIN);
}

void TIdentityRecognizerTest::TestMoiKrug() {
    DEFINE_NETWORK(MoiKrug);
    TestValidUrlWithNetwork("http://ikonyik.moikrug.ru/", "ikonyik", NSocial::PP_MOIKRUG);
    TestValidUrlWithNetwork("http://ikonyik.moikrug.ru", "ikonyik", NSocial::PP_MOIKRUG);
    TestInvalidUrl("http://ikonyik.moikrug.ru/?from=rss");
    TestValidUrlWithNetwork("http://moikrug.ru/anton", "anton", NSocial::PP_MOIKRUG);
    TestValidUrlWithNetwork("https://moikrug.ru/anton/", "anton", NSocial::PP_MOIKRUG);
    TestValidUrlWithNetwork("http://moikrug.ru/anton/friends", "anton", NSocial::PP_NONE);
    TestInvalidUrl("http://moikrug.ru/resumes");
    TestInvalidUrl("http://companies.moikrug.ru");
    TestInvalidUrl("http://companies.moikrug.ru/");
    TestInvalidUrl("http://moikrug.ru/users/P935676520");
    TestValidUrlWithNetwork("http://vasileVskiy.Moikrug.ru", "vasilevskiy", NSocial::PP_MOIKRUG);
    TestValidUrlWithNetwork("http://Pavel-Burangulov.moikrug.ru", "pavel-burangulov", NSocial::PP_MOIKRUG);
    TestValidUrlWithNetwork("http://Pavel_Burangulov.moikrug.ru", "pavel_burangulov", NSocial::PP_MOIKRUG);
    TestInvalidUrl("http://moikrug.ru/users/P314666109/foaf/?set=robot");
    TestInvalidUrl("http://moikrug.ru/users/P314666109/foaf/");
}

void TIdentityRecognizerTest::TestGooglePlus() {
    DEFINE_NETWORK(GooglePlus);
    TestValidUrlWithNetwork("http://plus.google.com/104560124403688998123/posts", "104560124403688998123", NSocial::PP_GPLUS);
    TestValidUrlWithNetwork("https://plus.google.com/u/0/115321208127905757827", "115321208127905757827", NSocial::PP_GPLUS);
    TestValidUrlWithNetwork("http://profiles.google.com/104956266026046411888/", "104956266026046411888", NSocial::PP_GPLUS);
    TestInvalidUrl("https://plus.google.com/u/0/111414336394941097843/?prsrc=3");
    TestValidUrlWithNetwork("http://plus.google.com/101773962078274680285/101773962078274680285", "101773962078274680285", NSocial::PP_GPLUS);
    TestValidUrlWithNetwork("https://plus.google.com/112845913669453565568#", "112845913669453565568", NSocial::PP_GPLUS);
    TestValidUrlWithNetwork("https://plus.google.com/108415216749874849490/plusones#", "108415216749874849490", NSocial::PP_GPLUS);
    TestInvalidUrl("https://plus.google.com/106802381013986482440/about?hl=ru&cx=rq4w56t2w");
    TestValidUrlWithNetwork("http://picasaweb.google.com/dirklindert", "dirklindert", NSocial::PP_NONE);
    TestValidUrlWithNetwork("http://www.google.com/reader/shared/11960499451169820915", "11960499451169820915", NSocial::PP_NONE);
    TestInvalidUrl("https://sites.google.com/site/ldersot/home?pli=1");
    TestInvalidUrl("https://play.google.com/store/apps/details?id=com.nikanorov.callnotes");
    TestInvalidUrl("http://www.google.com/sidewiki/feeds/entries/author/104808398875797406852/default");
}

void TIdentityRecognizerTest::TestLiveInternet() {
    DEFINE_NETWORK(LiveInternet);
    TestValidUrlWithNetwork("http://liveinternet.ru/users/3715422/blog#post192263502", "3715422", NSocial::PP_NONE);
    TestValidUrlWithNetwork("http://liveinternet.ru/users/3715422/blog", "3715422", NSocial::PP_NONE);
    TestValidUrlWithNetwork("http://www.liveinternet.ru/community/julia_serenova", "julia_serenova", NSocial::PP_NONE);
}

void TIdentityRecognizerTest::TestFreeLance() {
    DEFINE_NETWORK(FreeLance);
    TestInvalidUrl("http://www.free_lance.ru/users/botay");
    TestValidUrlWithNetwork("http://www.free-lance.ru/users/botay", "botay", NSocial::PP_FREELANCE);
    TestValidUrlWithNetwork("http://www.free-lance.ru/users/botay/", "botay", NSocial::PP_FREELANCE);
    TestValidUrlWithNetwork("http://www.free-lance.ru/users/BangBang/info", "bangbang", NSocial::PP_FREELANCE);
    TestInvalidUrl("http://www.free-lance.ru/users/BangBang/dfssfsdf");
    TestInvalidUrl("http://www.free-lance.ru/users/kleo2010/setup/info");
    TestValidUrlWithNetwork("http://free-lance.ru/users/NorStar/info", "norstar", NSocial::PP_FREELANCE);
    TestValidUrlWithNetwork("http://fl.ru/users/NorStar/info", "norstar", NSocial::PP_FREELANCE);
}

void TIdentityRecognizerTest::TestFriendFeed() {
    DEFINE_NETWORK(FriendFeed);
    TestInvalidUrl("http://friendfeed.com/list/tha7kat?auth=XKdSrjO2PoNg3oiH&format=atom");
    TestInvalidUrl("http://friendfeed.com/chuckkahn/likes");
    TestInvalidUrl("http://friendfeed.com/123suds/subscribers");
    TestValidUrlWithNetwork("http://friendfeed.com/christianbsas#", "christianbsas", NSocial::PP_FRIENDFEED);
    TestValidUrlWithNetwork("https://friendfeed.com/peperme#", "peperme", NSocial::PP_FRIENDFEED);
}

void TIdentityRecognizerTest::TestMirTesen() {
    DEFINE_NETWORK(MirTesen);
    TestValidUrlWithNetwork("http://mirtesen.ru/people/613435092", "613435092", NSocial::PP_MIRTESEN);
    TestInvalidUrl("http://mirtesen.ru/people/613435092?gam=logo");
    TestInvalidUrl("http://mirtesen.ru/people/649971527/friends");
    TestInvalidUrl("http://mirtesen.ru/people/792464850/applications");
    TestInvalidUrl("http://mirtesen.ru/people/649792121?gam://my.mail.ru/mail/cristin98/21?gam");
    TestInvalidUrl("http://mirtesen.ru/people/552182396%EC");
    TestInvalidUrl("http://mirtesen.ru/people/512943858;http");
}

void TIdentityRecognizerTest::TestFoursquare() {
    DEFINE_NETWORK(Foursquare);
    TestValidUrlWithNetwork("https://ru.foursquare.com/user/32343930", "user/32343930", NSocial::PP_FOURSQUARE);
    TestValidUrlWithNetwork("https://ru.foursquare.com/ya_abash", "ya_abash", NSocial::PP_FOURSQUARE);
    TestValidUrlWithNetwork("https://foursquare.com/ya_abash", "ya_abash", NSocial::PP_FOURSQUARE);
}

void TIdentityRecognizerTest::TestInstagram() {
    DEFINE_NETWORK(Instagram);
    TestInvalidUrl("http://instagram.com/ab-4S");
    TestValidUrlWithNetwork("http://instagram.com/CassieBrittain", "cassiebrittain", NSocial::PP_INSTAGRAM);
    TestInvalidUrl("http://twitter.com/explore/CassieBrittain");
}

class TProfileIdRecognizerTest: public TTestBase {
private:
    UNIT_TEST_SUITE(TProfileIdRecognizerTest)
        UNIT_TEST(TestProfileIdRecognizer);
    UNIT_TEST_SUITE_END();

public:
    void TestProfileIdRecognizer();
};

UNIT_TEST_SUITE_REGISTRATION(TProfileIdRecognizerTest);

#define UNIT_ASSERT_TRUE(A)  UNIT_ASSERT((A))
#define UNIT_ASSERT_FALSE(A) UNIT_ASSERT(!(A))

void TProfileIdRecognizerTest::TestProfileIdRecognizer() {
    using namespace NSocial;

    UNIT_ASSERT_FALSE(ValidateProfileId("123face123", NSocial::Facebook));
    UNIT_ASSERT_TRUE(ValidateProfileId("123", NSocial::Facebook));

    UNIT_ASSERT_FALSE(ValidateProfileId("", NSocial::VKontakte));
    UNIT_ASSERT_FALSE(ValidateProfileId("azer", NSocial::VKontakte));
    UNIT_ASSERT_TRUE(ValidateProfileId("id1", NSocial::VKontakte));

    UNIT_ASSERT_TRUE(ValidateProfileId("P123", NSocial::MoiKrug));
    UNIT_ASSERT_FALSE(ValidateProfileId("Pity", NSocial::MoiKrug));

    UNIT_ASSERT_TRUE(ValidateProfileId("2321", NSocial::GooglePlus));
    UNIT_ASSERT_FALSE(ValidateProfileId("1botay", NSocial::GooglePlus));

    UNIT_ASSERT_FALSE(ValidateProfileId("598hg3itg3j3-958jg3i5okg[35g35g", NSocial::LinkedIn));
    UNIT_ASSERT_TRUE(ValidateProfileId("abc/123", NSocial::LinkedIn));

    UNIT_ASSERT_TRUE(ValidateProfileId("profile/123", NSocial::Odnoklassniki));
    UNIT_ASSERT_FALSE(ValidateProfileId("1123", NSocial::Odnoklassniki));

    UNIT_ASSERT_TRUE(ValidateProfileId("1", NSocial::LiveJournal));
    UNIT_ASSERT_FALSE(ValidateProfileId("1b", NSocial::LiveJournal));


    UNIT_ASSERT_FALSE(ValidateProfileId("1", NSocial::YaRu));
    UNIT_ASSERT_FALSE(ValidateProfileId("1", NSocial::FreeLance));
    UNIT_ASSERT_FALSE(ValidateProfileId("1", NSocial::MoiMirMailRu));
}
