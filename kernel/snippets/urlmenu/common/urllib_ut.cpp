#include <library/cpp/testing/unittest/registar.h>
#include <util/string/vector.h>
#include "urllib.h"

Y_UNIT_TEST_SUITE(TSitelinksUrllibTest) {
    Y_UNIT_TEST(TestNormalize) {
        UNIT_ASSERT_VALUES_EQUAL(
            "www.mirknigi.ru/tag-1321.html?tn=%CE%F0%EB%EE%E2%E0+%EB.%E2%F1%B8+%EE+%EA%EE%F8%EA%E0%F5",
            NSitelinks::NormalizeUrl("http://www.mirknigi.ru/tag-1321.html?tn=%CE%F0%EB%EE%E2%E0+%EB.%E2%F1%B8+%EE+%EA%EE%F8%EA%E0%F5"));
        UNIT_ASSERT_VALUES_EQUAL(
            "www.e1.ru/", NSitelinks::NormalizeUrl("www.e1.ru"));
        UNIT_ASSERT_VALUES_EQUAL(
            "yandex.ru/", NSitelinks::NormalizeUrl("yandex.ru/"));
        UNIT_ASSERT_VALUES_EQUAL(
            "ru.wikipedia.org/wiki/%D0%9F%D0%BE%D1%80%D1%82%D0%B0%D0%BB:%D0%98%D1%81%D1%82%D0%BE%D1%80%D0%B8%D1%8F",
            NSitelinks::NormalizeUrl("http://ru.wikipedia.org/wiki/%D0%9F%D0%BE%D1%80%D1%82%D0%B0%D0%BB:%D0%98%D1%81%D1%82%D0%BE%D1%80%D0%B8%D1%8F"));
        UNIT_ASSERT_VALUES_EQUAL(
            "mris.net.ru/Component/Option,Com_Playfon_Html/Itemid,150/",
            NSitelinks::NormalizeUrl("http://Mris.Net.Ru/Component/Option,Com_Playfon_Html/Itemid,150/"));
        UNIT_ASSERT_VALUES_EQUAL(
            "www.yves-rocher.ru/Control/Category/~Category_Id=Cn3_Mix?Block=2",
            NSitelinks::NormalizeUrl("http://Www.Yves-Rocher.Ru/Control/Category/~Category_Id=Cn3_Mix?Block=2"));
        UNIT_ASSERT_VALUES_EQUAL(
            "ru.wikipedia.org/wiki/%D0%92%D0%B8%D0%BA%D0%B8%D0%BF%D0%B5%D0%B4%D0%B8%D1%8F:%D0%91%D1%8B%D1%81%D1%82%D1%80%D1%8B%D0%B9_%D0%B8%D0%BD%D0%B4%D0%B5%D0%BA%D1%81",
            NSitelinks::NormalizeUrl("http://ru.wikipedia.org/wiki/Википедия:Быстрый_индекс"));
        UNIT_ASSERT_VALUES_EQUAL(
            "tv.rbc.ru/?c147593g1", NSitelinks::NormalizeUrl("http://tv.rbc.ru/?c147593g1"));
        UNIT_ASSERT_VALUES_EQUAL(
            "www.playzeek.com/#index.php?key=g3.signup.1&gatherID=639030",
            NSitelinks::NormalizeUrl("www.playzeek.com/%23index.php%3Fkey%3Dg3.signup.1%26gatherID%3D639030"));
        UNIT_ASSERT_VALUES_EQUAL(
            "116.ru/job/", NSitelinks::NormalizeUrl("116.ru/job/"));
        UNIT_ASSERT_VALUES_EQUAL(
            "mskd.ru/games.php?id.4921.%CD%E5%E4%E5%F2%F1%EA%E8%E5_%E8%E3%F0%FB%3A_2+3=XXX_%282008%29",
            NSitelinks::NormalizeUrl("mskd.ru/games.php?id.4921.%CD%E5%E4%E5%F2%F1%EA%E8%E5_%E8%E3%F0%FB:_2+3=XXX_(2008)"));
    }

    Y_UNIT_TEST(TestPageParents) {
        TVector<TString> parents;

        NSitelinks::ListPageParents("www.mirknigi.ru/tag-1321.html?tn=%CE%F0%EB%EE%E2%E0+%EB.%E2%F1%B8+%EE+%EA%EE%F8%EA%E0%F5", parents);
        UNIT_ASSERT_VALUES_EQUAL("www.mirknigi.ru/tag-1321.html :: www.mirknigi.ru/", JoinStrings(parents, " :: "));

        NSitelinks::ListPageParents("http://ru.wikipedia.org/wiki/Википедия:Быстрый_индекс", parents);
        UNIT_ASSERT_VALUES_EQUAL("ru.wikipedia.org/wiki/ :: ru.wikipedia.org/", JoinStrings(parents, " :: "));

        NSitelinks::ListPageParents("www.playzeek.com/%23index.php%3Fkey%3Dg3.signup.1%26gatherID%3D639030", parents);
        UNIT_ASSERT_VALUES_EQUAL("www.playzeek.com/", JoinStrings(parents, " :: "));

        NSitelinks::ListPageParents("http://www.66.ru/club/21/", parents);
        UNIT_ASSERT_VALUES_EQUAL("www.66.ru/club/ :: www.66.ru/", JoinStrings(parents, " :: "));
    }
}
