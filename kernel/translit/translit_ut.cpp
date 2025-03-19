/// author@ vvp@ Victor Ploshikhin
/// created: Nov 14, 2012 8:11:58 PM

#include "translit.h"
#include "transmap.h"
#include <util/charset/wide.h>
#include <library/cpp/testing/unittest/registar.h>

class TTranslitTest : public TTestBase {

    UNIT_TEST_SUITE(TTranslitTest);
        UNIT_TEST(TestBySymbol)
    UNIT_TEST_SUITE_END();

    void Test(const TString& text, const TString& result, const ELanguage textLanguage=LANG_RUS) const
    {
        const TString res = TransliterateBySymbol(text, textLanguage);
        Cerr << text << " -> " << res << Endl;
        UNIT_ASSERT_STRINGS_EQUAL(res, result);
    }

    void TestBySymbol()
    {
        Test("aa", "aa");
        Test("eбта", "ebta");
        Test("ёбта", "yobta");
        Test("Работает, блеать!", "Rabotaet, bleat'!");
        Test("Працює, блеать!", "Pracyuye, bleat'!", LANG_UKR);
        Test("işleri meleme", "isleri meleme", LANG_TUR);
        Test("groß", "groß", LANG_GER);
        Test("büyük", "büyük"); // for russian
        Test("büyük", "buyuk", LANG_TUR);
        Test("큰", "큰", LANG_KOR);
        Test("ероглиф 大", "eroglif 大");

        Test("Ѐ", "e");
        Test("100500", "100500");
    }
};


class TTranslitTableTest : public TTestBase {

    UNIT_TEST_SUITE(TTranslitTableTest);
        UNIT_TEST(TestByTable)
    UNIT_TEST_SUITE_END();

    void Test(const TString& text, const TString& control, const ELanguage textLanguage=LANG_RUS) const
    {
        TUtf16String wtext = UTF8ToWide(text);
        TString res;
        TableTranslitBySymbol(wtext, textLanguage, res);
        Cerr << text << " -> " << res << Endl;
        UNIT_ASSERT_STRINGS_EQUAL(res, control);
    }

    void TestByTable()
    {
        Test("aa", "aa");
        Test("Привет", "privet");
        Test("Мир, труд, май!", "mir, trud, maj!", LANG_RUS);
        Test("Мир, труд, май!", "mir, trud, maj!", LANG_UKR);
        Test("işleri meleme", "isleri meleme", LANG_TUR);
        Test("groß", "gross", LANG_GER);
        Test("büyük", "byk"); // for russian
        Test("büyük", "buyuk", LANG_TUR);
        Test("큰", "", LANG_KOR);
        Test("ероглиф 大", "eroglif ");
        Test("Ѐ", "");
        Test("100500", "100500");

    }
};

class TTranslitCustomTableTest : public TTestBase {

    UNIT_TEST_SUITE(TTranslitCustomTableTest);
        UNIT_TEST(TestByTable)
    UNIT_TEST_SUITE_END();

    void Test(const TString& text, const TString& control, const NTranslit::TCharTable& table = {}) const
    {
        TUtf16String wtext = UTF8ToWide(text);
        TString res;
        NTranslit::TableTranslitBySymbol(wtext, res, table);
        Cerr << text << " -> " << res << Endl;
        UNIT_ASSERT_STRINGS_EQUAL(res, control);
    }

    void TestByTable()
    {
        Test("Привет", "");
        Test("Привет", "privet", NTranslit::GetCyrillic());
        Test("Hello", "Hello");
        Test("groß", "gross", NTranslit::GetLatin());
        Test("groß", "gro", NTranslit::GetCyrillic());
        Test("абаваба", "1213121", {{1072, "1"}, {1073, "2"}, {1074, "3"}});
    }
};


UNIT_TEST_SUITE_REGISTRATION(TTranslitTest);
UNIT_TEST_SUITE_REGISTRATION(TTranslitTableTest);
UNIT_TEST_SUITE_REGISTRATION(TTranslitCustomTableTest);
