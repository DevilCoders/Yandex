#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/charset/wide.h>
#include <library/cpp/charset/recyr.hh>
#include <util/datetime/cputimer.h>
#include <util/system/tempfile.h>
#include <util/stream/output.h>
#include <library/cpp/deprecated/split/split_iterator.h>

#include <kernel/indexer/lexical_decomposition/token_lexical_splitter.h>
#include <kernel/indexer/lexical_decomposition/vocabulary_builder.h>

namespace {
    const TString LINES =
        "domisad домисад\n"
        "svadbavkazani свадьбавказани\n"
        "onclinicneva онклиникнева\n"
        "tvplus твплюс\n"
        "automaximum автомаксимум аутомаксимум\n"
        "lantatur лантатур лантатюр лантатурь\n"
        "automaster автомастер\n"
        "krolikikompania кроликикомпания\n"
        "azibuki азибуки\n"
        "romanianchurchofgod\n"
        "romanianchurchofgodplus\n"
        "romanianxyzplus домабвсад\n"
        "abbac\n";
    const TString RU_VOCABULARY =
        "как     86700   2           \n"
        "компания        1000    0   \n"
        "многих  1200    0           \n"
        "наших   2100    2           \n"
        "шла     900     0           \n"
        "аз      1000    0           \n"
        "икомпания       100     0   \n"
        "мастер  700     0           \n"
        "кролик  1000    0           \n"
        "казани  200     0           \n"
        "тюр     500     1           \n"
        "буки    1000    0           \n"
        "авто   2000    0\n"
        "нева    700     0           \n"
        "ауто    500     0           \n"
        "ведь    6700    0           \n"
        "ланта   100     1           \n"
        "кролики 500     0           \n"
        "турь    2323    1           \n"
        "тур     1000    0           \n"
        "клиник  500     0           \n"
        "максимум        700     0   \n"
        "свадьба 1000    0           \n"
        "дом    1000    0\n"
        "и  34834495    2\n"
        "сад    1000   0\n"
        "в  23840324    2\n"
        "казани 1000    0\n"
        "он     453453  2\n"
        "тв     32424   2\n"
        "плюс   1000    0\n"
        "т      32434   1\n"
        "вплюс  55422   1\n"
        "мастер 1000    0\n"
        "азиб   3242    1\n"
        "уки    23424   1\n"
        "доми   24234   1\n";
    const TString EN_VOCABULARY =
        "tv 1000    0\n"
        "plus   1000    0\n"
        "onclinic    342432  1\n"
        "on         1234324  2\n"
        "clinic     9834     0\n"
        "neva       3453     0\n"
        "auto       1000    0\n"
        "maximum    1000    0\n"
        "krolik     1000    0\n"
        "i          2347324 2\n"
        "kroliki     200    0\n"
        "kompania   900     0\n"
        "azibuki    12389   1\n"
        "romanian   1000    0\n"
        "church     1000    0\n"
        "of         1000    2\n"
        "god        1000    0\n"
        "abba       2000    0\n";

    using namespace NLexicalDecomposition;

    const ui32 TEST_CNT = 2;
    const ui32 OPTIONS[TEST_CNT] = {
        DO_DEFAULT, /// and RU_VOC only
        DO_DEFAULT  /// and with EN_VOC
    };
    const TString ANSWER[TEST_CNT] = {
        "domisad: доми сад\n"
        "svadbavkazani: свадьба в казани\n"
        "onclinicneva: он клиник нева\n"
        "tvplus: тв плюс\n"
        "automaximum: авто максимум\n"
        "lantatur: ланта тур\n"
        "automaster: авто мастер\n"
        "krolikikompania: кролики компания\n"
        "azibuki: аз и буки\n"
        "romanianchurchofgod:\n"
        "romanianchurchofgodplus:\n"
        "romanianxyzplus:\n"
        "abbac:\n",

        "domisad: доми сад\n"
        "svadbavkazani: свадьба в казани\n"
        "onclinicneva: onclinic neva\n"
        "tvplus: tv plus\n"
        "automaximum: авто максимум\n"
        "lantatur: ланта тур\n"
        "automaster: авто мастер\n"
        "krolikikompania: кролики компания\n"
        "azibuki: azibuki\n"
        "romanianchurchofgod: romanian church of god\n"
        "romanianchurchofgodplus:\n"
        "romanianxyzplus:\n"
        "abbac:\n",
    };

}

class TTestLexicalDecomposition: public TTestBase {
private:
    UNIT_TEST_SUITE(TTestLexicalDecomposition);
    UNIT_TEST(Test);
    UNIT_TEST(TestAbsentLanguage);
    //        UNIT_TEST(TestSpeed);
    UNIT_TEST_SUITE_END();

    TTestLexicalDecomposition() {
        Init();
    }

    void Test() {
        for (size_t i = 0; i < TEST_CNT; ++i) {
            const static TSplitDelimiters DELIMS("\n");
            const TDelimitersSplit splitInput(LINES, DELIMS);
            const TDelimitersSplit splitAnswer(ANSWER[i], DELIMS);
            TDelimitersSplit::TIterator itInput = splitInput.Iterator();
            TDelimitersSplit::TIterator itAnswer = splitAnswer.Iterator();
            while (!itInput.Eof()) {
                NLexicalDecomposition::TTokenLexicalSplitter splitter(Data[i], false, false, OPTIONS[i]);
                const TString inputWin = Recode(CODES_UTF8, CODES_WIN, itInput.NextString());
                TString resultWin = splitter.ProcessUntranslitOutput(inputWin) + ":";
                for (size_t j = 0; j < splitter.GetResultSize(); ++j) {
                    resultWin += ' ';
                    resultWin += WideToChar(splitter[j], CODES_YANDEX);
                }
                const TString result = Recode(CODES_WIN, CODES_UTF8, resultWin);
                const TString answer = itAnswer.NextString();
                if (result != answer)
                    ythrow yexception() << "failed test " << i + 1 << ". Expected: " << answer << ", found: " << result;
            }
        }
    }

    void TestSpeed() {
        static const ui32 ITERATIONS = 10000;
        TTimeLogger logger("");
        for (ui32 i = 0; i < ITERATIONS; ++i)
            Test();
        logger.SetOK();
    }

    void TestAbsentLanguage() {
        for (size_t i = 0; i < TEST_CNT; ++i) {
            NLexicalDecomposition::TTokenLexicalSplitter splitter(Data[i]);
            TUtf16String inp = u"clinic";
            UNIT_ASSERT(!splitter.ProcessToken(inp, LANG_UKR));
            UNIT_ASSERT(!splitter.ProcessToken(inp, LANG_UNK));
            UNIT_ASSERT(!splitter.ProcessToken(inp, LANG_GER));
            UNIT_ASSERT(splitter.ProcessToken(inp, LANG_ENG) == (i == 1));
        }
    }

private:
    TBlob Data[TEST_CNT];

    void Init() {
        for (size_t i = 0; i < TEST_CNT; ++i) {
            TTempFileHandle rusFile = TTempFileHandle::InCurrentDir("rus");
            {
                TFixedBufferFileOutput fOut(rusFile.Name());
                fOut << RU_VOCABULARY;
            }
            TTempFileHandle engFile = TTempFileHandle::InCurrentDir("eng");
            {
                TFixedBufferFileOutput fOut(engFile.Name());
                if (i + 1 == TEST_CNT) {
                    fOut << EN_VOCABULARY;
                }
            }

            NLexicalDecomposition::TMultiVocabularyBuilder voc(false);
            voc.LoadVocabulary(LANG_RUS, rusFile.Name(), CODES_UTF8);
            voc.LoadVocabulary(LANG_ENG, engFile.Name(), CODES_UTF8);
            Data[i] = voc.Save();
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TTestLexicalDecomposition);
