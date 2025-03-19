#include <library/cpp/testing/unittest/registar.h>

#include <kernel/translate/translate.h>

namespace {
class TTranslitTest : public TTestBase
{
    UNIT_TEST_SUITE(TTranslitTest);
        UNIT_TEST(TestRussianToEnglish);
        UNIT_TEST(TestRussianFromEnglish);
    UNIT_TEST_SUITE_END();
private:
    void Check(ELanguage language, bool from, const TVector<TString>& words, const TVector<TVector<TString>>& lemmas) {
        for (size_t i = 0; i < words.size(); ++i) {
            TUtf16String word = UTF8ToWide(words[i]);
            TWLemmaArray out;
            if (from) {
                NTranslate::FromEnglish(word, language, out, 10);
            } else {
                NTranslate::ToEnglish(word, language, out, 10);
            }
            UNIT_ASSERT_VALUES_EQUAL(out.size(), lemmas[i].size());
            for (size_t j = 0; j < lemmas[i].size(); ++j) {
                UNIT_ASSERT_VALUES_EQUAL(WideToUTF8(out[j].GetText()), lemmas[i][j]);
            }
        }
    }
public:
    void TestRussianToEnglish() {
        TVector<TString> words = {"мама", "мыла", "раму", "папа"};
        TVector<TVector<TString>> lemmas = {
            {"mummy"},
            {"soap", "wash"},
            {},
            {"dad", "pope"}
        };
        Check(LANG_RUS, false, words, lemmas);
    }
    void TestRussianFromEnglish() {
        TVector<TString> words = {"Fire", "Earth", "Brethren", "and" ,"Wolves", "Forgotten"};
        TVector<TVector<TString>> lemmas = {
            {"огонь", "пожар", "пожарный"},
            {"земля"},
            {"брат", "братец"},
            {"а", "и"},
            {"волк"},
            {"забывать", "забытый", "затерянный"}
        };
        Check(LANG_RUS, true, words, lemmas);
    }
};
}

UNIT_TEST_SUITE_REGISTRATION(TTranslitTest);
