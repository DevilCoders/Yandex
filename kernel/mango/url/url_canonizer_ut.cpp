#include <kernel/mango/url/url_canonizer.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NMango;

Y_UNIT_TEST_SUITE(TURLCanonizerTest) {
    static void CheckCanonization(const TString &input, const TString &expectedOutput) {
        TURLCanonizer c;
        UNIT_ASSERT_VALUES_EQUAL(c.Canonize(input), expectedOutput);
    }

    static void CheckCanonizationWitoutScheme(const TString &input, const TString &expectedOutput) {
        TURLCanonizer c(true, true, false);
        UNIT_ASSERT_VALUES_EQUAL(c.Canonize(input), expectedOutput);
    }


    Y_UNIT_TEST(TestCanonization) {
        CheckCanonization("http://www.microsoft.com/Default.aspx", "http://microsoft.com");
        CheckCanonization("https://www.ya.ru/", "http://ya.ru");
        CheckCanonization("http://ya.ru/123/", "http://ya.ru/123");
        CheckCanonization("http://www.yandex.ru/index.html", "http://yandex.ru");
        CheckCanonization("http://www.yandex.ru/abc/def/gfh/index.jsp", "http://yandex.ru/abc/def/gfh");
        CheckCanonization("https://porno.porno.porno", "http://porno.porno.porno");
        CheckCanonization("https://trac.openstreetmap.org/browser/applications/utils/export/osm2pgsql/gazetteer/reindex.php", "http://trac.openstreetmap.org/browser/applications/utils/export/osm2pgsql/gazetteer/reindex.php");
        CheckCanonization("http://example.com/index.html#fragment", "http://example.com/index.html#fragment");
    }

    Y_UNIT_TEST(TestCanonizationWithoutScheme) {
        CheckCanonizationWitoutScheme("http://www.microsoft.com/Default.aspx", "http://microsoft.com");
        CheckCanonizationWitoutScheme("https://www.ya.ru/", "https://ya.ru");
        CheckCanonizationWitoutScheme("http://ya.ru/123/", "http://ya.ru/123");
        CheckCanonizationWitoutScheme("http://www.yandex.ru/index.html", "http://yandex.ru");
        CheckCanonizationWitoutScheme("http://www.yandex.ru/abc/def/gfh/index.jsp", "http://yandex.ru/abc/def/gfh");
        CheckCanonizationWitoutScheme("https://porno.porno.porno", "https://porno.porno.porno");
        CheckCanonizationWitoutScheme("https://trac.openstreetmap.org/browser/applications/utils/export/osm2pgsql/gazetteer/reindex.php", "https://trac.openstreetmap.org/browser/applications/utils/export/osm2pgsql/gazetteer/reindex.php");
        CheckCanonizationWitoutScheme("http://example.com/index.html#fragment", "http://example.com/index.html#fragment");
    }


}
