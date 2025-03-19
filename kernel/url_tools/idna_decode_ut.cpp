#include "idna_decode.h"
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(IDNATest) {
    Y_UNIT_TEST(FullDecode) {
        TString utf8;
        IDNAUrlToUtf8("http://xn--80aealabqjf0ckffj9fi0e.xn--p1ai:8080/cherepovets", utf8, false);
        UNIT_ASSERT_EQUAL(utf8, "http://\xd0\xba\xd1\x80\xd0\xb0\xd1\x81\xd0\xb8\xd0\xb2\xd1\x8b\xd0\xb9\xd1\x87\xd0\xb5\xd1\x80\xd0\xb5\xd0\xbf\xd0\xbe\xd0\xb2\xd0\xb5\xd1\x86.\xd1\x80\xd1\x84:8080/cherepovets");
    }

    Y_UNIT_TEST(DecodeError) {
        TString utf8;
        UNIT_ASSERT_EQUAL(false, IDNAUrlToUtf8("http://xn--80aealab\xd0\xbaqjf0ckffj9fi0e.xn--p1ai:8080/cherepovets", utf8, false));
    }

    Y_UNIT_TEST(NoIdnaTest) {
        TString utf8;
        UNIT_ASSERT_EQUAL(false, IDNAUrlToUtf8("http://yandex.ru:8080/cherepovets", utf8, false));
    }

    Y_UNIT_TEST(CutScheme) {
        TString utf8;
        IDNAUrlToUtf8("http://xn--80aealabqjf0ckffj9fi0e.xn--p1ai:8080/cherepovets", utf8, true);
        UNIT_ASSERT_EQUAL(utf8, "\xd0\xba\xd1\x80\xd0\xb0\xd1\x81\xd0\xb8\xd0\xb2\xd1\x8b\xd0\xb9\xd1\x87\xd0\xb5\xd1\x80\xd0\xb5\xd0\xbf\xd0\xbe\xd0\xb2\xd0\xb5\xd1\x86.\xd1\x80\xd1\x84:8080/cherepovets");
    }

    Y_UNIT_TEST(WithoutScheme) {
        TString utf8;
        IDNAUrlToUtf8("xn--80aealabqjf0ckffj9fi0e.xn--p1ai:8080/cherepovets", utf8, true);
        UNIT_ASSERT_EQUAL(utf8, "\xd0\xba\xd1\x80\xd0\xb0\xd1\x81\xd0\xb8\xd0\xb2\xd1\x8b\xd0\xb9\xd1\x87\xd0\xb5\xd1\x80\xd0\xb5\xd0\xbf\xd0\xbe\xd0\xb2\xd0\xb5\xd1\x86.\xd1\x80\xd1\x84:8080/cherepovets");
    }

    Y_UNIT_TEST(NoHostSIevlevSegfault) {
        // It seems sievlev@ commit r3034225 only masks real problem in ParseUri(..) because is is ill-formed and should not
        // return as ParsedOK. But we add this testcase to be able to fix parser later.
        TString utf8;
        UNIT_ASSERT_EQUAL(false, IDNAUrlToUtf8("?????????????????encuen[pz.xn--onbbd<ets dio2$xK e te", utf8, false));
    }
}
