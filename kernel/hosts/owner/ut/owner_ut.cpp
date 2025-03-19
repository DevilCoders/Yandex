#include <library/cpp/testing/unittest/registar.h>
#include <kernel/hosts/owner/owner.h>


Y_UNIT_TEST_SUITE(TOwnerCanonizerTest) {
    Y_UNIT_TEST(SimpleTest) {
        TOwnerCanonizer oc;
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("yandex.ru"), "yandex.ru");
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("www.yandex.ru"), "yandex.ru");
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("mail.yandex.ru"), "yandex.ru");
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("www.mail.yandex.ru"), "yandex.ru");
    }
    Y_UNIT_TEST(WithDom2Test) {
        TOwnerCanonizer oc;
        oc.AddDom2("yandex.ru");
        oc.AddDom2("mail.yandex.com");
        oc.AddDom2("yandex.eu");
        oc.AddDom2("mail.yandex.eu");
        oc.AddDom2("extra.mail.yandex.eu");
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("yandex.ru"), "yandex.ru");
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("www.yandex.ru"), "www.yandex.ru");
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("mail.yandex.ru"), "mail.yandex.ru");
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("www.mail.yandex.ru"), "mail.yandex.ru");
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("mail.yandex.com"), "mail.yandex.com");
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("www.mail.yandex.com"), "www.mail.yandex.com");
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("yandex.eu"), "yandex.eu");
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("www.yandex.eu"), "www.yandex.eu");
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("mail.yandex.eu"), "mail.yandex.eu");
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("www.mail.yandex.eu"), "www.mail.yandex.eu");
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("extra.mail.yandex.eu"), "extra.mail.yandex.eu");
        UNIT_ASSERT_EQUAL(oc.GetHostOwner("www.extra.mail.yandex.eu"), "www.extra.mail.yandex.eu");
        UNIT_ASSERT_EQUAL(oc.InDom2("yandex.ru"), true);
        UNIT_ASSERT_EQUAL(oc.InDom2("www.yandex.ru"), true);
        UNIT_ASSERT_EQUAL(oc.InDom2("www.www.yandex.ru"), true);
        UNIT_ASSERT_EQUAL(oc.InDom2("yandex.eu"), true);
        UNIT_ASSERT_EQUAL(oc.InDom2("www.yandex.net"), false);
        UNIT_ASSERT_EQUAL(oc.InDom2("yandex.net"), false);
        UNIT_ASSERT_EQUAL(oc.InDom2("yandex.com"), false);
    }
    Y_UNIT_TEST(UrlTest) {
        TOwnerCanonizer oc;
        UNIT_ASSERT_EQUAL(oc.GetUrlOwner("yandex.ru"), "yandex.ru");
        UNIT_ASSERT_EQUAL(oc.GetUrlOwner("www.yandex.ru/some/path"), "yandex.ru");
        UNIT_ASSERT_EQUAL(oc.GetUrlOwner("yandex.ru:888"), "yandex.ru");
        UNIT_ASSERT_EQUAL(oc.GetUrlOwner("http://yandex.ru:888"), "yandex.ru");
        UNIT_ASSERT_EQUAL(oc.GetUrlOwner("http://www.yandex.ru:888/some/path"), "yandex.ru");
        UNIT_ASSERT_EQUAL(oc.GetUrlOwner("https://secret.dom.ru/"), "dom.ru");
    }

    Y_UNIT_TEST(CompiledTest) {
        TOwnerCanonizer c;
        UNIT_ASSERT_VALUES_EQUAL(c.GetHostOwner("budda.com.ua"), "com.ua");
        CompiledDom2(c, NOwner::AREAS_LST_FILENAME);
        UNIT_ASSERT_VALUES_EQUAL(c.GetHostOwner("budda.com.ua"), "budda.com.ua");
        CompiledDom2(c, NOwner::AREAS_LST_FILENAME, false);
        UNIT_ASSERT_VALUES_EQUAL(c.GetHostOwner("budda.com.ua"), "com.ua");
        UNIT_ASSERT_EXCEPTION(CompiledDom2(c, "foo.bar"), yexception);
    }

    Y_UNIT_TEST(CompiledSpbTest) {
        TOwnerCanonizer c;
        CompiledDom2(c, NOwner::AREAS_LST_FILENAME);
        UNIT_ASSERT_VALUES_EQUAL(c.GetHostOwner("www.spb.ru"), "www.spb.ru");
        UNIT_ASSERT_VALUES_EQUAL(c.GetHostOwner("gov.spb.ru"), "gov.spb.ru");
        UNIT_ASSERT_VALUES_EQUAL(c.GetHostOwner("www.lenta.gov.spb.ru"), "lenta.gov.spb.ru");
        UNIT_ASSERT_VALUES_EQUAL(c.GetHostOwner("test.peter.gov.spb.ru"), "test.peter.gov.spb.ru");
        UNIT_ASSERT_VALUES_EQUAL(c.GetHostOwner("www.test.peter.gov.spb.ru"), "test.peter.gov.spb.ru");
    }

    Y_UNIT_TEST(GetNextHostSuffixOrOwnerFuncTest) {
        HashSet h;
        h.Add("com.ua");

        {
            TStringBuf url = "";
            UNIT_ASSERT_VALUES_EQUAL(GetNextHostSuffixOrOwner(h, url), false);
        }

        {
            TStringBuf url = "a";
            UNIT_ASSERT_VALUES_EQUAL(GetNextHostSuffixOrOwner(h, url), false);
        }

        {
            TStringBuf url = "a.b";
            UNIT_ASSERT_VALUES_EQUAL(GetNextHostSuffixOrOwner(h, url), false);
        }

        {
            TStringBuf url = "a.b.c";
            UNIT_ASSERT_VALUES_EQUAL(GetNextHostSuffixOrOwner(h, url), true);
            UNIT_ASSERT_VALUES_EQUAL(url, "b.c");
            UNIT_ASSERT_VALUES_EQUAL(GetNextHostSuffixOrOwner(h, url), false);
        }

        {
            TStringBuf url = "foo.bar.com.ua";
            UNIT_ASSERT_VALUES_EQUAL(GetNextHostSuffixOrOwner(h, url), true);
            UNIT_ASSERT_VALUES_EQUAL(url, "bar.com.ua");
            UNIT_ASSERT_VALUES_EQUAL(GetNextHostSuffixOrOwner(h, url), false);
        }
    }

    Y_UNIT_TEST(GetNextHostSuffixOrOwnerCompiledTest) {
        TOwnerCanonizer c;
        {
            TStringBuf url = "budda.com.ua";
            UNIT_ASSERT_VALUES_EQUAL(c.GetNextHostSuffixOrOwner(url), true);
        }
        CompiledDom2(c, NOwner::AREAS_LST_FILENAME);
        {
            TStringBuf url = "budda.com.ua";
            UNIT_ASSERT_VALUES_EQUAL(c.GetNextHostSuffixOrOwner(url), false);
        }
    }
}
