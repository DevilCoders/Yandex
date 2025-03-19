#include <kernel/snippets/titles/make_title/make_title.h>

#include <kernel/qtree/richrequest/richnode.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NSnippets {

TString DoTestMakeTitleNoQuery(const TString& utf8Text, int maxPixelLength, float fontSize) {
    TUtf16String text = UTF8ToWide(utf8Text);
    TUtf16String result = MakeTitleString(text, nullptr, maxPixelLength, fontSize);
    return WideToUTF8(result);
}

TString DoTestMakeTitle(const TString& utf8Text, const TString& utf8Query, int maxPixelLength, float fontSize) {
    TUtf16String text = UTF8ToWide(utf8Text);
    TRichTreePtr tree = CreateRichTree(UTF8ToWide(utf8Query), TCreateTreeOptions(LANG_RUS));
    TUtf16String result = MakeTitleString(text, tree->Root.Get(), maxPixelLength, fontSize);
    return WideToUTF8(result);
}

Y_UNIT_TEST_SUITE(TMakeTitleTests) {
    Y_UNIT_TEST(TestMakeTitleString) {
        TString odnokl = "Одноклассники";
        UNIT_ASSERT_STRINGS_EQUAL(DoTestMakeTitleNoQuery(odnokl, 200, 18), odnokl);
        UNIT_ASSERT_STRINGS_EQUAL(DoTestMakeTitleNoQuery(odnokl, 10, 18), "");

        TString orenb = "Работа в Оренбурге, подбор персонала, резюме, вакансии, "
            "советы по трудоустройству - поиск работы на orenburg.rabota.ru";
        UNIT_ASSERT_STRINGS_EQUAL(DoTestMakeTitleNoQuery(orenb, 300, 18),
            "Работа в Оренбурге, подбор...");
        UNIT_ASSERT_STRINGS_EQUAL(DoTestMakeTitleNoQuery(orenb, 300, 13),
            "Работа в Оренбурге, подбор персонала...");
        UNIT_ASSERT_STRINGS_EQUAL(DoTestMakeTitleNoQuery(orenb, 1000, 18),
            "Работа в Оренбурге, подбор персонала, резюме, вакансии, советы по трудоустройству - поиск работы на...");

        TString defin = "MMOSHOP.RU | Купить CS GO случайное оружие";
        UNIT_ASSERT_STRINGS_EQUAL(DoTestMakeTitleNoQuery(defin, 400, 18),
            "MMOSHOP.RU | Купить CS GO случайное...");

        TString book = "Архив еврейской истории. Том 3 > 5-8243-0753-9 > "
            "5-8243-0599-4 > 2006 > Купить > Украина > Цена > Киев > Магазин книг «Езоп»";
        UNIT_ASSERT_STRINGS_EQUAL(DoTestMakeTitle(book, "zzz", 250, 18),
            "Архив еврейской истории.");
        UNIT_ASSERT_STRINGS_EQUAL(DoTestMakeTitle(book, "zzz", 400, 18),
            "Архив еврейской истории.");

        TString tur = "Ücretsiz Online Çeviri";
        UNIT_ASSERT_STRINGS_EQUAL(DoTestMakeTitleNoQuery(tur, 300, 18), tur);
        UNIT_ASSERT_STRINGS_EQUAL(DoTestMakeTitleNoQuery(tur, 150, 18), "Ücretsiz Online...");

        TString wiki = "Новый год — Википедия";
        UNIT_ASSERT_STRINGS_EQUAL(DoTestMakeTitleNoQuery(wiki, 300, 18), wiki);
        UNIT_ASSERT_STRINGS_EQUAL(DoTestMakeTitleNoQuery(wiki, 150, 18), "Новый год...");
        UNIT_ASSERT_STRINGS_EQUAL(DoTestMakeTitle(wiki, "новый год википедия", 200, 18), "Новый год...");

        TString chars1 = ":: МОБИЧАТ - Мобильный чат - О чате ::";
        UNIT_ASSERT_STRINGS_EQUAL(DoTestMakeTitleNoQuery(chars1, 600, 18),
            "МОБИЧАТ - Мобильный чат - О чате");

        TString chars2 = "[CHEAT CSS] чит на css v85 (YouTube)";
        UNIT_ASSERT_STRINGS_EQUAL(DoTestMakeTitleNoQuery(chars2, 600, 18), chars2);
    }
}

}
