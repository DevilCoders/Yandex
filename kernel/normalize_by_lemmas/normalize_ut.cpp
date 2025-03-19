#include "normalize.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>

Y_UNIT_TEST_SUITE(PhoneTests) {
    Y_UNIT_TEST(PhoneLeadingPlusSevenElevenDigits) {
        const TString src = "+7 958 6543210";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, "79586543210");
    }

    Y_UNIT_TEST(PhoneLeadingPlusSevenOnlyDigits) {
        const TString src = "+79586543210";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, "79586543210");
    }

    Y_UNIT_TEST(PhoneLeadingPlusSevenTenDigits) {
        const TString src = "+7 958 654321";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, src);
    }

    Y_UNIT_TEST(PhoneLeadingPlusSevenTwelveDigits) {
        const TString src = "+7 958 65432109";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, "795865432109");
    }

    Y_UNIT_TEST(PhoneLeadingSevenElevenDigits) {
        const TString src = "7 958 6543210";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, "79586543210");
    }

    Y_UNIT_TEST(PhoneLeadingSevenOnlyDigits) {
        const TString src = "79586543210";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, "79586543210");
    }

    Y_UNIT_TEST(PhoneLeadingEightElevenDigits) {
        const TString src = "8999-123-45-45";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, "89991234545");
    }

    Y_UNIT_TEST(PhoneLeadingEightOnlyDigits) {
        const TString src = "89991234545";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, "89991234545");
    }

    Y_UNIT_TEST(PhoneLeadingPlusEightElevenDigits) {
        const TString src = "+8999-123-45-45";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, src);
    }

    Y_UNIT_TEST(PhoneLeadingPlusEightOnlyDigits) {
        const TString src = "+89991234545";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, "+8999123454 5");
    }

    Y_UNIT_TEST(PhoneLeadingEightPlusAmong) {
        const TString src = "8999-123-45+45";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, src);
    }

    Y_UNIT_TEST(PhoneLeadingEightTenDigits) {
        const TString src = "8999-123-45-4";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, src);
    }

    Y_UNIT_TEST(PhoneLeadingEightAlienCharacters) {
        const TString src = "8999-123-45-r45";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, src);
    }

    Y_UNIT_TEST(PhoneWithLeadingAndTrailingLetters) {
        const TString src = "bbb 8999-123-45-45 aaa";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, "bbb 89991234545 aaa");
    }

    Y_UNIT_TEST(NotPhoneWithLetters) {
        const TString src = "abc 7(999)123 b la 3999-123-45-45";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, src);
    }

    Y_UNIT_TEST(PhoneNotPhoneWithLetters) {
        const TString src = "abc 2(999)123-45-45 b la 8999-123-45-45";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, "abc 2(999)123-45-45 b la 89991234545");
    }

    Y_UNIT_TEST(PhoneNotPhoneDigitsOnly) {
        const TString src = "79991234567 39991234545";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, "79991234567 3999123454 5");
    }

    Y_UNIT_TEST(NotPhonePhoneDigitsOnly) {
        const TString src = "39991234567 89991234545";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, "3999123456 7 89991234545");
    }

    Y_UNIT_TEST(TwoNotPhonesPhoneDigitsOnly) {
        const TString src = "39991234567 39991234567 89991234545";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, "3999123456 7 3999123456 7 89991234545");
    }

    Y_UNIT_TEST(NotPhoneHyphenPhone) {
        const TString src = "3-9991234567 8(999)1234545";
        const TString dst = NormalizePhones(src);
        UNIT_ASSERT_STRINGS_EQUAL(dst, "3-9991234567 89991234545");
    }

    Y_UNIT_TEST(IsPhoneLeadingEightOnlyDigits) {
        const TString src = "89991234545";
        UNIT_ASSERT(DoesContainPhone(src));
    }

    Y_UNIT_TEST(IsPhoneLeadingEight) {
        const TString src = "8(999)123-45-45";
        UNIT_ASSERT(DoesContainPhone(src));
    }

    Y_UNIT_TEST(IsPhoneLeadingPlusSevenOnlyDigits) {
        const TString src = "+79991234545";
        UNIT_ASSERT(DoesContainPhone(src));
    }

    Y_UNIT_TEST(IsPhoneLeadingSevenOnlyDigits) {
        const TString src = "79991234545";
        UNIT_ASSERT(DoesContainPhone(src));
    }

    Y_UNIT_TEST(NotPhoneForeignOnlyDigits) {
        const TString src = "39991234545";
        UNIT_ASSERT(!DoesContainPhone(src));
    }

    Y_UNIT_TEST(NotPhoneBangladeshOnlyDigits) {
        const TString src = "+88002000600";
        UNIT_ASSERT(!DoesContainPhone(src));
    }

    Y_UNIT_TEST(NotPhoneLettersAmong) {
        const TString src = "8800f2000600";
        UNIT_ASSERT(!DoesContainPhone(src));
    }

    Y_UNIT_TEST(NotPhoneTooFewDigits) {
        const TString src = "8800200060";
        UNIT_ASSERT(!DoesContainPhone(src));
    }
}
