#include "extract_headline.h"

#include <library/cpp/resource/resource.h>
#include <library/cpp/scheme/scheme.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NFactsSerpParser {

    static TString GetFactHeadline(const TStringBuf& resourceKey) {
        const TString json = NResource::Find(resourceKey);
        const NSc::TValue value = NSc::TValue::FromJsonThrow(json);

        const TString headline = ExtractHeadline(value);
        return headline;
    }

    Y_UNIT_TEST_SUITE(ExtractHeadline) {
        Y_UNIT_TEST(AllFactTypes) {
            UNIT_ASSERT_STRINGS_EQUAL(GetFactHeadline("calories_fact"), "Калорийность Яблоко. Химический состав и пищевая ценность.");
            UNIT_ASSERT_STRINGS_EQUAL(GetFactHeadline("dict_fact"), "Сапог - это... Что такое сапог?");
            UNIT_ASSERT_STRINGS_EQUAL(GetFactHeadline("entity_fact"), "");
            UNIT_ASSERT_STRINGS_EQUAL(GetFactHeadline("poetry_lover"), "");
            UNIT_ASSERT_STRINGS_EQUAL(GetFactHeadline("suggest_fact"), "Амазонка — Википедия");
            UNIT_ASSERT_STRINGS_EQUAL(GetFactHeadline("table_fact"), "Список пустынь по площади — Википедия");
            UNIT_ASSERT_STRINGS_EQUAL(GetFactHeadline("znatoki_fact"), "Что такое вселенная?");
            UNIT_ASSERT_STRINGS_EQUAL(GetFactHeadline("math_fact"), "");
        }
    }

} // namespace NFactsSerpParser
