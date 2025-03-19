#include <kernel/common_server/util/raw_text/datetime.h>

#include <library/cpp/testing/unittest/registar.h>

#include <kernel/common_server/util/raw_text/phone_number.h>

Y_UNIT_TEST_SUITE(PhoneNumberParseSuite) {
    Y_UNIT_TEST(RussiaPhoneNumber) {
        TPhoneNormalizer normalizer;

        TString expectedNumber = "+79060200822";

        TString rawNumber;

        rawNumber = "+79060200822";
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));

        rawNumber = "+7 (906) 020 08-22";
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));

        rawNumber = "9060200822";  // local Russia number, 10 digits
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));

        rawNumber = "79060200822";  // local Russia number with country code
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));

        rawNumber = "89060200822";  // local Russia number with 8 prefix
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));

        rawNumber = "879060200822";  // local Russia number with 8 prefix and coutry code
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));

        rawNumber = "8109060200822";  // local Russia number with 8 10 prefix
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));

        rawNumber = "81079060200822";  // local Russia number with 8 10 prefix and coutry code
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));

        rawNumber = "989060200822";  // local Russia number with 9 8 prefix
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));

        rawNumber = "9879060200822";  // local Russia number with 9 8 prefix and coutry code
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));

        rawNumber = "98109060200822";  // local Russia number with 9 8 10 prefix
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));

        rawNumber = "981079060200822";  // local Russia number with 9 8 10 prefix and coutry code
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));

        rawNumber = "981089060200822";  // local Russia number with 9 8 10 8 prefix
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));

        rawNumber = "9810879060200822";  // local Russia number with 9 8 10 8 prefix and coutry code
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));

        rawNumber = "9881079060200822";  // local Russia number with 9 8 8 10 prefix
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));
    }

    Y_UNIT_TEST(PossibleParsingErrorsPhoneNumber) {
        TPhoneNormalizer normalizer;

        UNIT_ASSERT_STRINGS_EQUAL("+982112345678", normalizer.TryNormalize("+982112345678")); // Iran
        UNIT_ASSERT_STRINGS_EQUAL("+982112345678", normalizer.TryNormalize("982112345678")); // Iran

        UNIT_ASSERT_STRINGS_EQUAL("+995350123456", normalizer.TryNormalize("+995350123456")); // Georgia
        UNIT_ASSERT_STRINGS_EQUAL("+995350123456", normalizer.TryNormalize("995350123456")); // Georgia

        UNIT_ASSERT_STRINGS_EQUAL("+81345102539", normalizer.TryNormalize("+81345102539")); // Japan
        UNIT_ASSERT_STRINGS_EQUAL("+81345102539", normalizer.TryNormalize("9881345102539")); // Japan
        UNIT_ASSERT_STRINGS_EQUAL("+81345102539", normalizer.TryNormalize("981081345102539")); // Japan

        UNIT_ASSERT_STRINGS_EQUAL("+19177500243", normalizer.TryNormalize("+19177500243")); // USA
        UNIT_ASSERT_STRINGS_EQUAL("+19177500243", normalizer.TryNormalize("19177500243")); // USA
        UNIT_ASSERT_STRINGS_EQUAL("+19177500243", normalizer.TryNormalize("9819177500243")); // USA

        UNIT_ASSERT_STRINGS_EQUAL("+79060200822", normalizer.TryNormalize("+789060200822")); // Invalid Russia
    }

    Y_UNIT_TEST(GermanyPhoneNumber) {
        TPhoneNormalizer normalizer;

        TString expectedNumber = "+491520547268";

        TString rawNumber;

        rawNumber = "+491520547268";
        UNIT_ASSERT_STRINGS_EQUAL(expectedNumber, normalizer.TryNormalize(rawNumber));
    }
}
