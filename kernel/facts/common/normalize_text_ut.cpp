#include "normalize_text.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NUnstructuredFeatures;

Y_UNIT_TEST_SUITE(NormalizeText) {
    Y_UNIT_TEST(LowercaseTest) {
        TUtf16String query1 = u"ТеКсТ ЗаПрОсА 1";
        TUtf16String normQuery1 = u"текст запроса 1";
        TUtf16String query2 = u"Some TEXT";
        TUtf16String normQuery2 = u"some text";
        UNIT_ASSERT(NormalizeText(query1) == normQuery1);
        UNIT_ASSERT(NormalizeText(query2) == normQuery2);
    }

    Y_UNIT_TEST(AlphaNumericTest) {
        TUtf16String query = u"?? 1, 2, три!!!";
        TUtf16String normQuery = u"1 2 три";
        UNIT_ASSERT(NormalizeText(query) == normQuery);
    }

    Y_UNIT_TEST(SpacesTest) {
        TUtf16String query = u" 1   2 1 4324  ";
        TUtf16String normQuery = u"1 2 1 4324";
        UNIT_ASSERT(NormalizeText(query) == normQuery);
    }

    Y_UNIT_TEST(StressMarkTest) {
        TUtf16String query = u"Прокрастина́ция - это ...";
        TUtf16String normQuery = u"прокрастинация это";
        UNIT_ASSERT(NormalizeText(query) == normQuery);
    }

    Y_UNIT_TEST(YoTest) {
        TUtf16String query = u"ЕЩЁ ещё";
        TUtf16String normQuery = u"ещё ещё";
        UNIT_ASSERT(NormalizeText(query) == normQuery);
    }

    Y_UNIT_TEST(MinusSignTest) {
        TUtf16String query = u"-1 - 2 = -3";
        TUtf16String normQuery = u"1 2 3";
        UNIT_ASSERT(NormalizeText(query) == normQuery);
    }

    Y_UNIT_TEST(MinusWordTest) {
        TUtf16String query1 = u"залоченный это -айфон";
        TUtf16String normQuery1 = u"залоченный это айфон";
        TUtf16String query2 = u"залоченный это -айфон-телефон";
        TUtf16String normQuery2 = u"залоченный это айфон телефон";
        TUtf16String query3 = u"залоченный это -айфон телефон";
        TUtf16String normQuery3 = u"залоченный это айфон телефон";
        TUtf16String query4 = u"залоченный это -айфон -телефон";
        TUtf16String normQuery4 = u"залоченный это айфон телефон";
        TUtf16String query5 = u"залоченный это - айфон-телефон";
        TUtf16String normQuery5 = u"залоченный это айфон телефон";
        UNIT_ASSERT(NormalizeText(query1) == normQuery1);
        UNIT_ASSERT(NormalizeText(query2) == normQuery2);
        UNIT_ASSERT(NormalizeText(query3) == normQuery3);
        UNIT_ASSERT(NormalizeText(query4) == normQuery4);
        UNIT_ASSERT(NormalizeText(query5) == normQuery5);
    }

    Y_UNIT_TEST(UTF8Test) {
        TString query = "Прокрастина́ция - это ... 123 4  5!!";
        TString normQuery = "прокрастинация это 123 4 5";
        UNIT_ASSERT_STRINGS_EQUAL(NormalizeTextUTF8(query), normQuery);
    }
}
