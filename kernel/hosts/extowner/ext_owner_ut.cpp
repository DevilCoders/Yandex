#include <library/cpp/testing/unittest/registar.h>
#include "ext_owner.h"

Y_UNIT_TEST_SUITE(TOwnerExtractorTest) {
    Y_UNIT_TEST(TestTLDs) {
        TVector<TString> areas;
        TOwnerExtractor ownerExtractor(areas);

        #define TEST_CASE(a, b)  UNIT_ASSERT_EQUAL(ownerExtractor.GetOwner(a), TStringBuf(b))

        TEST_CASE("foo.ru", "foo.ru");
        TEST_CASE("foo.Ru", "foo.Ru");
        TEST_CASE("foo.foo.BAR", "foo.BAR");
        TEST_CASE("foo.foo.XN--p1aI", "foo.XN--p1aI");
        TEST_CASE("foo.foo.b", "foo.foo.b");
        TEST_CASE("foo.foo.XN--", "foo.foo.XN--");

        #undef TEST_CASE
    }

    Y_UNIT_TEST(TestTrivial) {
        TVector<TString> areas;
        TOwnerExtractor ownerExtractor(areas);

        #define TEST_CASE(a, b)  UNIT_ASSERT_EQUAL(ownerExtractor.GetOwner(a), TStringBuf(b))

        TEST_CASE("yandex.ru", "yandex.ru");
        TEST_CASE("www.yandex.ru", "yandex.ru");
        TEST_CASE("mail.yandex.ru", "yandex.ru");
        TEST_CASE("www.mail.yandex.ru", "yandex.ru");
        TEST_CASE("enlace_externo_ejemplo.com", "enlace_externo_ejemplo.com");
        TEST_CASE("www.ru", "www.ru");
        TEST_CASE("www.yandex", "www.yandex");

        #undef TEST_CASE
    }

    Y_UNIT_TEST(TestIP) {
        TVector<TString> areas;
        TOwnerExtractor ownerExtractor(areas);

        #define TEST_CASE(a, b)  UNIT_ASSERT_EQUAL(ownerExtractor.GetOwner(a), TStringBuf(b))

        TEST_CASE("127.0.0.1:80/", "127.0.0.1");
        TEST_CASE("127.0.0.1", "127.0.0.1");
        TEST_CASE("[2a02:6b8:0:3400::1071]:80/", "[2a02:6b8:0:3400::1071]");
        TEST_CASE("[::1]", "[::1]");

        #undef TEST_CASE
    }

    Y_UNIT_TEST(TestComplex1) {
        TVector<TString> areas;
        // Areas which are owners too
        areas.push_back("foo.ru");
        areas.push_back("jar.ru");
        areas.push_back("foo.jar.ru");
        // An area which is not an owner
        areas.push_back("foo.bar.ru");
        TOwnerExtractor ownerExtractor(areas);

        #define TEST_CASE(a, b)  UNIT_ASSERT_EQUAL(ownerExtractor.GetOwner(a), TStringBuf(b))

        TEST_CASE("foo.ru", "foo.ru");
        TEST_CASE("bar.foo.ru", "bar.foo.ru");
        TEST_CASE("http://www.bar.foo.ru", "bar.foo.ru");
        TEST_CASE("www.bar.foo.ru:80", "bar.foo.ru");
        TEST_CASE("HTTP://www.bar.foo.ru:80", "bar.foo.ru");
        TEST_CASE("jar.Ru", "jar.Ru");
        TEST_CASE("foo.Jar.ru", "foo.Jar.ru");
        TEST_CASE("bar.Foo.jar.ru", "bar.Foo.jar.ru");
        TEST_CASE("www.Bar.foo.jar.ru", "Bar.foo.jar.ru");
        TEST_CASE("bar.Ru", "bar.Ru");
        TEST_CASE("foo.Bar.ru", "foo.Bar.ru");
        TEST_CASE("jar.Foo.bar.ru", "jar.Foo.bar.ru");

        #undef TEST_CASE
    }

    Y_UNIT_TEST(TestComplex2) {
        TVector<TString> areas;
        areas.push_back("yandex.ru");
        areas.push_back("mail.yandex.com");
        areas.push_back("yandex.eu");
        areas.push_back("mail.yandex.eu");
        areas.push_back("extra.mail.yandex.eu");
        TOwnerExtractor ownerExtractor(areas);

        #define TEST_CASE(a, b)  UNIT_ASSERT_EQUAL(ownerExtractor.GetOwner(a), TStringBuf(b))

        TEST_CASE("yandex.ru", "yandex.ru");
        TEST_CASE("www.yandex.ru", "www.yandex.ru");
        TEST_CASE("mail.yandex.ru", "mail.yandex.ru");
        TEST_CASE("www.mail.yandex.ru", "mail.yandex.ru");
        TEST_CASE("mail.yandex.com", "mail.yandex.com");
        TEST_CASE("www.mail.yandex.com", "www.mail.yandex.com");
        TEST_CASE("yandex.eu", "yandex.eu");
        TEST_CASE("www.yandex.eu", "www.yandex.eu");
        TEST_CASE("mail.yandex.eu", "mail.yandex.eu");
        TEST_CASE("www.mail.yandex.eu", "www.mail.yandex.eu");
        TEST_CASE("extra.mail.yandex.eu", "extra.mail.yandex.eu");
        TEST_CASE("www.extra.mail.yandex.eu", "www.extra.mail.yandex.eu");
        TEST_CASE("www.yandex.ru/some/path", "www.yandex.ru");
        TEST_CASE("yandex.ru:888", "yandex.ru");
        TEST_CASE("http://yandex.ru:888", "yandex.ru");
        TEST_CASE("http://www.yandex.ru:888/some/path", "www.yandex.ru");
        TEST_CASE("https://secret.dom.ru/", "dom.ru");

        #undef TEST_CASE
    }

    Y_UNIT_TEST(TestComplexShortHost) {
        TVector<TString> areas;
        areas.push_back("google.ru");
        areas.push_back("g.ru");
        TOwnerExtractor ownerExtractor(areas);

        #define TEST_CASE(a, b)  UNIT_ASSERT_EQUAL(ownerExtractor.GetOwner(a), TStringBuf(b))

        TEST_CASE("www.google.ru", "www.google.ru");
        TEST_CASE("www.g.ru", "www.g.ru");
        TEST_CASE("www.google.me", "google.me");
        TEST_CASE("www.g.me", "g.me");

        #undef TEST_CASE
    }

    Y_UNIT_TEST(TestPathPrefix) {
        TVector<TString> areas;
        areas.push_back("li.ru/users/ [a-z0-9\\.\\-_]+");
        areas.push_back("li.ru/USERS/ [A-Z0-9\\.\\-_]+");
        areas.push_back("raw:([a-z0-9\\-]+\\.)?mail\\.ru/(u|U)sers/ [a-zA-Z0-9\\.\\-_]+");
        areas.push_back("at.tut.by");
        areas.push_back("at.tut.by/ [a-zA-Z0-9\\.\\-_]+");
        TOwnerExtractor ownerExtractor(areas);

        #define TEST_CASE(a, b)  UNIT_ASSERT_EQUAL(ownerExtractor.GetOwner(a), TStringBuf(b))

        TEST_CASE("li.ru", "li.ru");
        TEST_CASE("li.ru/users", "li.ru");
        TEST_CASE("li.ru/users/", "li.ru");
        TEST_CASE("li.ru/users?foo", "li.ru");
        TEST_CASE("li.ru/users/vasya", "li.ru/users/vasya");
        TEST_CASE("li.ru/users/vasya/foo", "li.ru/users/vasya");
        TEST_CASE("li.ru/users/VASYA", "li.ru");
        TEST_CASE("li.ru/USERS/VASYA", "li.ru/USERS/VASYA");
        TEST_CASE("li.ru/USERS/VASYA/foo", "li.ru/USERS/VASYA");
        TEST_CASE("li.ru/USERS/vasya", "li.ru");
        TEST_CASE("my.mail.ru/users/vasya", "mail.ru/users/vasya");
        TEST_CASE("HZ.Mail.Ru/Users/Vasya", "Mail.Ru/Users/Vasya");
        TEST_CASE("mail.ru/users/vasya/foo", "mail.ru/users/vasya");
        TEST_CASE("http://li.ru/users", "li.ru");
        TEST_CASE("http://www.li.ru/users/vasya=badass", "li.ru/users/vasya");
        TEST_CASE("http://li.ru/users?foo", "li.ru");
        TEST_CASE("http://at.tut.by/vasya", "at.tut.by/vasya");

        #undef TEST_CASE
    }

    Y_UNIT_TEST(TestOwnerFromParts) {
        TVector<TString> areas;
        areas.push_back("li.ru/users/ [a-zA-Z0-9\\.\\-_]+");
        TOwnerExtractor ownerExtractor(areas);

        #define TEST_CASE(a, b, c, d) UNIT_ASSERT_EQUAL(ownerExtractor.GetOwnerFromParts(a, b), std::make_pair(TStringBuf(c), TStringBuf(d)))

        TEST_CASE("li.ru", "", "li.ru", "");
        TEST_CASE("li.ru", "/users", "li.ru", "");
        TEST_CASE("li.ru", "/users/", "li.ru", "");
        TEST_CASE("li.ru", "/users?foo", "li.ru", "");
        TEST_CASE("li.ru", "/users/vasya", "li.ru", "/users/vasya");
        TEST_CASE("li.ru", "/users/vasya/foo", "li.ru", "/users/vasya");
        TEST_CASE("http://li.ru", "/users", "li.ru", "");
        TEST_CASE("http://li.ru", "/users/", "li.ru", "");
        TEST_CASE("http://li.ru", "/users?foo", "li.ru", "");

        #undef TEST_CASE
    }

    Y_UNIT_TEST(TestNormalized) {
        TVector<TString> areas;
        areas.push_back("li.ru/users/ [a-zA-Z0-9\\.\\-_]+");
        TOwnerExtractor ownerExtractor(areas);

        #define TEST_CASE(a, b)  UNIT_ASSERT_EQUAL(ownerExtractor.GetOwnerNormalized(a), TString(b))

        TEST_CASE("LI.ru", "li.ru");
        TEST_CASE("LI.RU/users/vasya/foo", "li.ru/users/vasya");
        TEST_CASE("HTTP://LI.RU/users/VASYA", "li.ru/users/VASYA");
        TEST_CASE("http://li.ru/users/?FOO", "li.ru");
        TEST_CASE("FOO.BAR.JAR/goo", "bar.jar");
        TEST_CASE("FOO.BAR.J/goo", "foo.bar.j");

        #undef TEST_CASE
    }

    Y_UNIT_TEST(TestExtendable) {
        TVector<TString> areas;
        areas.push_back("foo.ru");
        areas.push_back("foo.bar.ru");
        areas.push_back("bar.bar.ru");
        areas.push_back("raw:((foo|jar)\\.)?bar\\.ru/(users|community)/ [a-zA-Z0-9\\.\\-_]+");
        TOwnerExtractor ownerExtractor(areas);

        #define TEST_CASE(a, b)  UNIT_ASSERT_EQUAL(ownerExtractor.IsExtendable(a), b)

        TEST_CASE("foo.ru", false);
        TEST_CASE("bar.ru", true);
        TEST_CASE("www.jar.ru", false);
        TEST_CASE("foo.bar.ru", true);
        TEST_CASE("bar.bar.ru", false);
        TEST_CASE("jar.bar.ru", true);

        #undef TEST_CASE
    }

    Y_UNIT_TEST(TestRobotLangExtention) {
        // NOTE https://wiki.yandex-team.ru/robot/multidocurls
        TVector<TString> areas;
        areas.push_back("foo.ru");
        TOwnerExtractor ownerExtractor(areas);

        #define TEST_CASE(a, b)  UNIT_ASSERT_EQUAL(ownerExtractor.GetOwner(a), TStringBuf(b))

        TEST_CASE("@us.bar.ru", "bar.ru");
        // TEST_CASE("@de.foo.ru", "foo.ru"); // right, but not worked now
        TEST_CASE("@de.https://foo.ru", "foo.ru");
        TEST_CASE("@de.subhost.foo.ru", "subhost.foo.ru");
        TEST_CASE("@tur.https://subhost.foo.ru", "subhost.foo.ru");

        #undef TEST_CASE
    }

    Y_UNIT_TEST(TestSaveLoad) {
        TVector<TString> areas{
            // Areas which are owners too
            "foo.ru",
            "jar.ru",
            "foo.jar.ru",
            // An area which is not an owner
            "foo.bar.ru",
        };

        TString serialized;
        {
            TStringOutput output(serialized);
            TOwnerExtractor(areas).Save(&output);
        }

        TOwnerExtractor loaded("");
        TStringInput input(serialized);
        loaded.Load(&input);

        #define TEST_CASE(url, expected_owner) \
            UNIT_ASSERT_EQUAL(loaded.GetOwner(url), TStringBuf(expected_owner))

        TEST_CASE("foo.ru", "foo.ru");
        TEST_CASE("bar.foo.ru", "bar.foo.ru");
        TEST_CASE("http://www.bar.foo.ru", "bar.foo.ru");
        TEST_CASE("www.bar.foo.ru:80", "bar.foo.ru");
        TEST_CASE("HTTP://www.bar.foo.ru:80", "bar.foo.ru");
        TEST_CASE("jar.Ru", "jar.Ru");
        TEST_CASE("foo.Jar.ru", "foo.Jar.ru");
        TEST_CASE("bar.Foo.jar.ru", "bar.Foo.jar.ru");
        TEST_CASE("www.Bar.foo.jar.ru", "Bar.foo.jar.ru");
        TEST_CASE("bar.Ru", "bar.Ru");
        TEST_CASE("foo.Bar.ru", "foo.Bar.ru");
        TEST_CASE("jar.Foo.bar.ru", "jar.Foo.bar.ru");

        #undef TEST_CASE
    }
}
