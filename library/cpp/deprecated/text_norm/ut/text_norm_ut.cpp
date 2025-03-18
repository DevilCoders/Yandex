#include <library/cpp/deprecated/omni/text_norm.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>

Y_UNIT_TEST_SUITE(OmnidexNormalizeTextSuite) {
    Y_UNIT_TEST(TestOmnidexNormalizeText) {
        UNIT_ASSERT_STRINGS_EQUAL(
            WideToUTF8(NOmni::NormalizeText(u" 1")),
            "1"
        );
        UNIT_ASSERT_STRINGS_EQUAL(
            WideToUTF8(NOmni::NormalizeText(u" 2 ")),
            "2"
        );
        UNIT_ASSERT_STRINGS_EQUAL(
            WideToUTF8(NOmni::NormalizeText(u"3 ")),
            "3"
        );
        UNIT_ASSERT_STRINGS_EQUAL(
            WideToUTF8(NOmni::NormalizeText(u"Eng")),
            "eng"
        );
        UNIT_ASSERT_STRINGS_EQUAL(
            WideToUTF8(NOmni::NormalizeText(u"Рус")),
            "рус"
        );
        UNIT_ASSERT_STRINGS_EQUAL(
            WideToUTF8(NOmni::NormalizeText(u"ONE\ttwo   Three")),
            "one two three"
        );
        UNIT_ASSERT_STRINGS_EQUAL(
            WideToUTF8(NOmni::NormalizeText(u"BİR\tiki   Üç")),
            "bir iki üç"
        );
        UNIT_ASSERT_STRINGS_EQUAL(
            WideToUTF8(NOmni::NormalizeText(u"a!b@c 1.2,3 ()[]{}")),
            "a b c 1 2 3"
        );
        UNIT_ASSERT_STRINGS_EQUAL(
            WideToUTF8(NOmni::NormalizeText(u"Санкт-Петербург")),
            "санкт петербург"
        );
        UNIT_ASSERT_STRINGS_EQUAL(
            WideToUTF8(NOmni::NormalizeText(u"Агент007")),
            "агент007"
        );
    }
}
