#include <kernel/mango/url/info.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TUrlInfoTest) {
    Y_UNIT_TEST(IsValidTest) {
        UNIT_ASSERT(!NMango::TUrlInfo("URL").IsValid());
        UNIT_ASSERT(!NMango::TUrlInfo("http:/  s//").IsValid());

        TString mediumSizePath(512, 'a');
        UNIT_ASSERT(NMango::TUrlInfo("http://ya.ru/" + mediumSizePath).IsValid());

        TString longPath(4*1024, 'a');
        UNIT_ASSERT(!NMango::TUrlInfo("http://ya.ru/" + longPath).IsValid());
    }

    Y_UNIT_TEST(IsTinyTest) {
        UNIT_ASSERT(!NMango::TUrlInfo("res://dsf").IsTiny());
        UNIT_ASSERT(!NMango::TUrlInfo("http://www.ya.ru/").IsTiny());
        UNIT_ASSERT(NMango::TUrlInfo("http://bit.ly/Ds54sdf").IsTiny());
        UNIT_ASSERT(NMango::TUrlInfo("http://goo.gl/sdf").IsTiny());
        UNIT_ASSERT(NMango::TUrlInfo("http://goo.gl/").IsTiny()); // morda is tiny also
        UNIT_ASSERT(!NMango::TUrlInfo("http://vkontakte.ru/id144051417").IsTiny());
        UNIT_ASSERT(!NMango::TUrlInfo("res://ieframe.dll/navcancl.htm").IsTiny());
        UNIT_ASSERT(NMango::TUrlInfo("http://feeds.feedburner.com/~r/Ikosmetika/~3/5ejjsFj0tK4/").IsTiny());
        UNIT_ASSERT(NMango::TUrlInfo("http://rss.feedsportal.com/c/32327/f/441153/s/1a57f7f7/l/0L0Srbc0Bru0Crbcfreenews0Bshtml0D0C20A1111240A0A29550Bshtml/story01.htm").IsTiny());
    }

    Y_UNIT_TEST(IsTrashTest) {
        UNIT_ASSERT(NMango::TUrlInfo("http://...badhost.ru/qwer").IsTrash());
        UNIT_ASSERT(NMango::TUrlInfo("http://bad_host2.ru/qwer").IsTrash());
        UNIT_ASSERT(NMango::TUrlInfo("httpqsd://this$is__-upyachka.url/qwer#55").IsTrash());
        UNIT_ASSERT(NMango::TUrlInfo("http://rss2lj.net/some").IsTrash());
        UNIT_ASSERT(!NMango::TUrlInfo("http://tema.livejournal.com").IsTrash());
        UNIT_ASSERT(!NMango::TUrlInfo("http://ria.ru").IsTrash());
        UNIT_ASSERT(NMango::TUrlInfo("http://goo.gl/").IsTrash()); // tiny morda is trash
        UNIT_ASSERT(NMango::TUrlInfo("ftp://91.227.69.2/Install/").IsTrash());
        UNIT_ASSERT(!NMango::TUrlInfo("https://www.windowsphone.com/ru-RU/my").IsTrash());
    }

    Y_UNIT_TEST(IsDeprecatedTest) {
        UNIT_ASSERT(NMango::TUrlInfo("http://news.yandex.ru").IsDeprecated());
        UNIT_ASSERT(NMango::TUrlInfo("http://news.yandex.ru/yandsearch?cl4url=www.ria.ru%2Fincidents%2F20110806%2F412712843.html&cat=0&lang=ru").IsDeprecated());
        UNIT_ASSERT(NMango::TUrlInfo("https://www.foursquare.com/some_page").IsDeprecated());
        UNIT_ASSERT(!NMango::TUrlInfo("http://tema.livejournal.com").IsDeprecated());
        UNIT_ASSERT(!NMango::TUrlInfo("http://ria.ru").IsDeprecated());
        UNIT_ASSERT(!NMango::TUrlInfo("http://vkontakte.ru").IsDeprecated());
        UNIT_ASSERT(NMango::TUrlInfo("http://vkontakte.ru/album/1234").IsDeprecated());
        UNIT_ASSERT(NMango::TUrlInfo("http://top.rbc.ru/").IsDeprecated());
        UNIT_ASSERT(!NMango::TUrlInfo("http://top.rbc.ru/economics/31/01/2012/635666.shtml").IsDeprecated());
    }

    Y_UNIT_TEST(IsMordaTest) {
        UNIT_ASSERT(NMango::TUrlInfo("http://www.ya.ru/").IsMorda());
        UNIT_ASSERT(NMango::TUrlInfo("http://ya.ru").IsMorda());
        UNIT_ASSERT(!NMango::TUrlInfo("http://www.ya.ru/something").IsMorda());
        UNIT_ASSERT(NMango::TUrlInfo("http://example.com/#something").IsMorda());
        UNIT_ASSERT(NMango::TUrlInfo("http://www.yandex.ru").IsMorda());
        UNIT_ASSERT(NMango::TUrlInfo("http://www.yandex.ru/").IsMorda());
        UNIT_ASSERT(NMango::TUrlInfo("http://username.livejournal.com").IsMorda());
        UNIT_ASSERT(!NMango::TUrlInfo("http:/asdasdsdsdsd").IsMorda());
        UNIT_ASSERT(!NMango::TUrlInfo("http://www.yandex.ru/index.html").IsMorda());
        UNIT_ASSERT(!NMango::TUrlInfo("http://example.test/?page=0").IsMorda());
        UNIT_ASSERT(!NMango::TUrlInfo("http://www.yandex.ru/index.html?key=value").IsMorda());
        UNIT_ASSERT(!NMango::TUrlInfo("http://www.yandex.ru/index.html?key=value&key2=value2").IsMorda());
    }
    Y_UNIT_TEST(IsInTest) {
        TVector< std::pair<TString, TString> > patterns = {
            std::make_pair("*", "/any/star"),
            std::make_pair("sample.ru", "/"),
            std::make_pair("ya.ru", "/some/path"),
            std::make_pair("*.wikipedia.org", "*"),
            std::make_pair("*znakomstva.*", "*")
        };
        UNIT_ASSERT(!NMango::TUrlInfo("http://www.ttt.ru/any/wrong").IsIn(patterns));
        UNIT_ASSERT(NMango::TUrlInfo("http://www.qqq.ttt.ru/any/star").IsIn(patterns));
        UNIT_ASSERT(NMango::TUrlInfo("http://www.sample.ru/").IsIn(patterns));
        UNIT_ASSERT(!NMango::TUrlInfo("http://ya.ru/").IsIn(patterns));
        UNIT_ASSERT(NMango::TUrlInfo("http://ya.ru/some/path").IsIn(patterns));
        UNIT_ASSERT(NMango::TUrlInfo("http://ru.wikipedia.org/something").IsIn(patterns));
        UNIT_ASSERT(NMango::TUrlInfo("http://en.wikipedia.org/").IsIn(patterns));
        UNIT_ASSERT(!NMango::TUrlInfo("http://wikipedia.org/").IsIn(patterns));
        UNIT_ASSERT(NMango::TUrlInfo("http://vbnmmnbvznakomstva.ru").IsIn(patterns));
        UNIT_ASSERT(NMango::TUrlInfo("http://ghjkkjhgznakomstva.com").IsIn(patterns));
        UNIT_ASSERT(NMango::TUrlInfo("http://znakomstva.net/win_free_ipad").IsIn(patterns));
        UNIT_ASSERT(!NMango::TUrlInfo("http://znakomstvaaaa.net/win_free_ipad").IsIn(patterns));
    }

}

