#include <library/cpp/testing/unittest/registar.h>

#include "url_canonizer.h"

#include <util/system/tempfile.h>

Y_UNIT_TEST_SUITE(TCanonizerTest) {
    Y_UNIT_TEST(TPunycodeHostCanonizerTest) {
        TPunycodeHostCanonizer c;
        UNIT_ASSERT_EQUAL("h", c.CanonizeHost("h"));
        UNIT_ASSERT_EQUAL("", c.CanonizeHost(""));
        UNIT_ASSERT_EQUAL("", c.CanonizeHost("h\xa0z"));
        UNIT_ASSERT_EQUAL("xn--d1abbgf6aiiy.xn--p1ai", c.CanonizeHost("президент.рф"));
    }

    Y_UNIT_TEST(TLowerCaseHostCanonizerTest) {
        TLowerCaseHostCanonizer c;
        UNIT_ASSERT_EQUAL("abcd", c.CanonizeHost("abcd"));
        UNIT_ASSERT_EQUAL("abcd", c.CanonizeHost("aBcD"));
    }

    Y_UNIT_TEST(TOwnerHostCanonizerTest) {
        TTempFileHandle tempFileHandle;
        {
            TFileOutput out(tempFileHandle.Name());
            out << "livejournal.com" << Endl;
        }

        TString name = tempFileHandle.Name();
        TOwnerHostCanonizer ownerCanonizer(name);
        UNIT_ASSERT_VALUES_EQUAL(ownerCanonizer.CanonizeHost("www.yandex.ru"), "yandex.ru");
        UNIT_ASSERT_VALUES_EQUAL(ownerCanonizer.CanonizeHost("yandex.ru"), "yandex.ru");
        UNIT_ASSERT_VALUES_EQUAL(ownerCanonizer.CanonizeHost("livejournal.com"), "livejournal.com");
        UNIT_ASSERT_VALUES_EQUAL(ownerCanonizer.CanonizeHost("user.livejournal.com"), "user.livejournal.com");
        UNIT_ASSERT_VALUES_EQUAL(ownerCanonizer.CanonizeHost("www.user.livejournal.com"), "user.livejournal.com");
        UNIT_ASSERT_VALUES_EQUAL(ownerCanonizer.CanonizeHost("http://www.user.livejournal.com"), "user.livejournal.com");
        UNIT_ASSERT_VALUES_EQUAL(ownerCanonizer.CanonizeHost("https://www.user.livejournal.com:8080"), "user.livejournal.com:8080");
    }
};
