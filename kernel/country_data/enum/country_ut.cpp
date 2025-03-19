#include "iso.h"
#include "region.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/strbuf.h>
#include <util/generic/typetraits.h>
#include <util/generic/xrange.h>
#include <util/stream/output.h>

using namespace NCountry;

Y_UNIT_TEST_SUITE(TCountryTests) {
    Y_UNIT_TEST(TestIfEnumerationValuesAreConsecutive) {
        for (const auto index: xrange(ECountry_ARRAYSIZE)) {
            UNIT_ASSERT(ECountry_IsValid(index));
        }
    }

    Y_UNIT_TEST(TestAlphaTwoCodes) {
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaTwoCode(C_TURKEY), TStringBuf("tr"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaTwoCode(C_KAZAKHSTAN), TStringBuf("kz"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaTwoCode(C_UKRAINE), TStringBuf("ua"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaTwoCode(C_BELARUS), TStringBuf("by"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaTwoCode(C_RUSSIA), TStringBuf("ru"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaTwoCode(C_GERMANY), TStringBuf("de"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaTwoCode(C_INDONESIA), TStringBuf("id"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaTwoCode(C_UNITED_STATES), TStringBuf("us"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaTwoCode(C_CHINA), TStringBuf("cn"));
    }

    Y_UNIT_TEST(TestAlphaThreeCodes) {
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaThreeCode(C_TURKEY), TStringBuf("tur"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaThreeCode(C_KAZAKHSTAN), TStringBuf("kaz"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaThreeCode(C_UKRAINE), TStringBuf("ukr"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaThreeCode(C_BELARUS), TStringBuf("blr"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaThreeCode(C_RUSSIA), TStringBuf("rus"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaThreeCode(C_GERMANY), TStringBuf("deu"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaThreeCode(C_INDONESIA), TStringBuf("idn"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaThreeCode(C_UNITED_STATES), TStringBuf("usa"));
        UNIT_ASSERT_VALUES_EQUAL(ToAlphaThreeCode(C_CHINA), TStringBuf("chn"));
    }

    Y_UNIT_TEST(TestToString) {
        UNIT_ASSERT_VALUES_EQUAL(ToString(C_TURKEY), TStringBuf("tr"));
        UNIT_ASSERT_VALUES_EQUAL(ToString(C_KAZAKHSTAN), TStringBuf("kz"));
        UNIT_ASSERT_VALUES_EQUAL(ToString(C_UKRAINE), TStringBuf("ua"));
        UNIT_ASSERT_VALUES_EQUAL(ToString(C_BELARUS), TStringBuf("by"));
        UNIT_ASSERT_VALUES_EQUAL(ToString(C_RUSSIA), TStringBuf("ru"));
        UNIT_ASSERT_VALUES_EQUAL(ToString(C_GERMANY), TStringBuf("de"));
        UNIT_ASSERT_VALUES_EQUAL(ToString(C_INDONESIA), TStringBuf("id"));
        UNIT_ASSERT_VALUES_EQUAL(ToString(C_UNITED_STATES), TStringBuf("us"));
        UNIT_ASSERT_VALUES_EQUAL(ToString(C_CHINA), TStringBuf("cn"));
    }

    Y_UNIT_TEST(TestFromString) {
        UNIT_ASSERT_VALUES_EQUAL(FromString<ECountry>("tur"), C_TURKEY);
        UNIT_ASSERT_VALUES_EQUAL(FromString<ECountry>("kaz"), C_KAZAKHSTAN);
        UNIT_ASSERT_VALUES_EQUAL(FromString<ECountry>("ukr"), C_UKRAINE);
        UNIT_ASSERT_VALUES_EQUAL(FromString<ECountry>("blr"), C_BELARUS);
        UNIT_ASSERT_VALUES_EQUAL(FromString<ECountry>("rus"), C_RUSSIA);
        UNIT_ASSERT_VALUES_EQUAL(FromString<ECountry>("deu"), C_GERMANY);
        UNIT_ASSERT_VALUES_EQUAL(FromString<ECountry>("idn"), C_INDONESIA);
        UNIT_ASSERT_VALUES_EQUAL(FromString<ECountry>("usa"), C_UNITED_STATES);
        UNIT_ASSERT_VALUES_EQUAL(FromString<ECountry>("chn"), C_CHINA);
    }

    Y_UNIT_TEST(TestRegionIds) {
        UNIT_ASSERT_VALUES_EQUAL(ToRegionId(C_TURKEY), 983);
        UNIT_ASSERT_VALUES_EQUAL(ToRegionId(C_KAZAKHSTAN), 159);
        UNIT_ASSERT_VALUES_EQUAL(ToRegionId(C_UKRAINE), 187);
        UNIT_ASSERT_VALUES_EQUAL(ToRegionId(C_BELARUS), 149);
        UNIT_ASSERT_VALUES_EQUAL(ToRegionId(C_RUSSIA), 225);
        UNIT_ASSERT_VALUES_EQUAL(ToRegionId(C_GERMANY), 96);
        UNIT_ASSERT_VALUES_EQUAL(ToRegionId(C_INDONESIA), 10095);
        UNIT_ASSERT_VALUES_EQUAL(ToRegionId(C_UNITED_STATES), 84);
        UNIT_ASSERT_VALUES_EQUAL(ToRegionId(C_CHINA), 134);
    }
}
