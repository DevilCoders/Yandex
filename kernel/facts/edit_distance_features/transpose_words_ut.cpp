#include "transpose_words.h"

#include <library/cpp/tokenizer/split.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>

#include <iterator>

using namespace NEditDistanceFeatures;

using TWordLemmaPairVector = TVector<TWordLemmaPair>;
using TLemmaMap = TWordLemmaPair::TLemmaArrayMap;

namespace {

    std::pair<TWordLemmaPairVector, TWordLemmaPairVector> MakeWordLemmaPtrArrays(const TVector<TUtf16String>& lhWords, const TVector<TUtf16String>& rhWords, TLemmaMap& lemmaCache, const TLangMask& langs) {
        auto wordToWordLemma = [&lemmaCache, &langs](const TUtf16String& word) {
            return TWordLemmaPair::GetCachedWordLemmaPair(word, lemmaCache, langs);
        };
        TVector<TWordLemmaPair> lhsWordLemmas;
        lhsWordLemmas.reserve(lhWords.size());
        Transform(lhWords.begin(), lhWords.end(), std::back_inserter(lhsWordLemmas), wordToWordLemma);
        TVector<TWordLemmaPair> rhsWordLemmas;
        lhsWordLemmas.reserve(rhWords.size());
        Transform(rhWords.begin(), rhWords.end(), std::back_inserter(rhsWordLemmas), wordToWordLemma);
        return std::make_pair(std::move(lhsWordLemmas), std::move(rhsWordLemmas));
    }

    bool AreLemmasAlike(const TWordLemmaPairVector& lhsWordLemmas, const TWordLemmaPairVector& rhsWordLemmas) {
        if (lhsWordLemmas.size() != rhsWordLemmas.size()) {
            return false;
        }

        auto lemmasAlike = [](const TWordLemmaPair& lhLemma, const TWordLemmaPair& rhLemma) {
            return lhLemma.Like(rhLemma);
        };
        return Equal(lhsWordLemmas.cbegin(), lhsWordLemmas.cend(), rhsWordLemmas.cbegin(), lemmasAlike);
    }

}

Y_UNIT_TEST_SUITE(TransposeWordsTests) {
    TLemmaMap LemmaCache;

    Y_UNIT_TEST(EmptyStrings) {
        auto lhWords = SplitIntoTokens(u"", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(u"", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, LemmaCache, TLangMask(LANG_RUS, LANG_ENG));
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas.size(), 0);
        UNIT_ASSERT_VALUES_EQUAL(rhLemmas.size(), 0);

        size_t transpositionCount = 0;
        size_t duplicateCount = 0;
        std::tie(transpositionCount, duplicateCount) = CountAndMakeWordLemmaTranspositions(lhLemmas, rhLemmas);
        UNIT_ASSERT_VALUES_EQUAL(transpositionCount, 0);
        UNIT_ASSERT(lhLemmas == rhLemmas);
    }

    Y_UNIT_TEST(EqualStrings) {
        auto lhWords = SplitIntoTokens(u"один два три", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(u"один два три", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, LemmaCache, TLangMask(LANG_RUS, LANG_ENG));
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas.size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(rhLemmas.size(), 3);

        size_t transpositionCount = 0;
        size_t duplicateCount = 0;
        std::tie(transpositionCount, duplicateCount) = CountAndMakeWordLemmaTranspositions(lhLemmas, rhLemmas);
        UNIT_ASSERT_VALUES_EQUAL(transpositionCount, 0);
        UNIT_ASSERT(lhLemmas == rhLemmas);
    }

    Y_UNIT_TEST(NoDuplicates) {
        auto lhWords = SplitIntoTokens(u"four five six", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(u"один два три", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, LemmaCache, TLangMask(LANG_RUS, LANG_ENG));
        UNIT_ASSERT(lhLemmas != rhLemmas);
        size_t transpositionCount = 0;
        size_t duplicateCount = 0;
        std::tie(transpositionCount, duplicateCount) = CountAndMakeWordLemmaTranspositions(lhLemmas, rhLemmas);
        UNIT_ASSERT_VALUES_EQUAL(transpositionCount, 0);
        UNIT_ASSERT(lhLemmas != rhLemmas);
    }

    Y_UNIT_TEST(TransposeEndToMiddle) {
        auto lhWords = SplitIntoTokens(u"один три два", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(u"один два три", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, LemmaCache, TLangMask(LANG_RUS, LANG_ENG));
        UNIT_ASSERT(lhLemmas != rhLemmas);
        size_t transpositionCount = 0;
        size_t duplicateCount = 0;
        std::tie(transpositionCount, duplicateCount) = CountAndMakeWordLemmaTranspositions(lhLemmas, rhLemmas);
        UNIT_ASSERT_VALUES_EQUAL(transpositionCount, 1);
        UNIT_ASSERT(lhLemmas == rhLemmas);
    }

    Y_UNIT_TEST(TransposeEndToBeginning) {
        auto lhWords = SplitIntoTokens(u"два три один", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(u"один два три", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, LemmaCache, TLangMask(LANG_RUS, LANG_ENG));
        UNIT_ASSERT(lhLemmas != rhLemmas);
        size_t transpositionCount = 0;
        size_t duplicateCount = 0;
        std::tie(transpositionCount, duplicateCount) = CountAndMakeWordLemmaTranspositions(lhLemmas, rhLemmas);
        UNIT_ASSERT_VALUES_EQUAL(transpositionCount, 1);
        UNIT_ASSERT(lhLemmas == rhLemmas);
    }

    Y_UNIT_TEST(SwapBeginningAndEnd) {
        auto lhWords = SplitIntoTokens(u"три два один", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(u"один два три", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, LemmaCache, TLangMask(LANG_RUS, LANG_ENG));
        UNIT_ASSERT(lhLemmas != rhLemmas);
        size_t transpositionCount = 0;
        size_t duplicateCount = 0;
        std::tie(transpositionCount, duplicateCount) = CountAndMakeWordLemmaTranspositions(lhLemmas, rhLemmas);
        UNIT_ASSERT_VALUES_EQUAL(transpositionCount, 2);
        UNIT_ASSERT(lhLemmas == rhLemmas);
    }

    Y_UNIT_TEST(SwapBeginningAndEndWordForms) {
        auto lhWords = SplitIntoTokens(u"трём двумя одного", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(u"один два три", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, LemmaCache, TLangMask(LANG_RUS, LANG_ENG));
        UNIT_ASSERT(lhLemmas != rhLemmas);
        size_t transpositionCount = 0;
        size_t duplicateCount = 0;
        std::tie(transpositionCount, duplicateCount) = CountAndMakeWordLemmaTranspositions(lhLemmas, rhLemmas);
        UNIT_ASSERT_VALUES_EQUAL(transpositionCount, 2);
        UNIT_ASSERT(AreLemmasAlike(lhLemmas, rhLemmas));
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[0].GetWord(), u"одного");
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[2].GetWord(), u"трём");
    }

    Y_UNIT_TEST(KeepWordForms) {
        auto lhWords = SplitIntoTokens(u"одного двумя трём", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(u"один два три", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, LemmaCache, TLangMask(LANG_RUS, LANG_ENG));
        UNIT_ASSERT(lhLemmas != rhLemmas);
        size_t transpositionCount = 0;
        size_t duplicateCount = 0;
        std::tie(transpositionCount, duplicateCount) = CountAndMakeWordLemmaTranspositions(lhLemmas, rhLemmas);
        UNIT_ASSERT_VALUES_EQUAL(transpositionCount, 0);
        UNIT_ASSERT_VALUES_EQUAL(duplicateCount, 3);
        UNIT_ASSERT(AreLemmasAlike(lhLemmas, rhLemmas));
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[0].GetWord(), u"одного");
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[1].GetWord(), u"двумя");
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[2].GetWord(), u"трём");
    }

    Y_UNIT_TEST(TransposeBeginningToEndOtherWords) {
        auto lhWords = SplitIntoTokens(u"музыка mp3 скачать", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(u"скачать музыку эмпэтри", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, LemmaCache, TLangMask(LANG_RUS, LANG_ENG));
        size_t transpositionCount = 0;
        size_t duplicateCount = 0;
        std::tie(transpositionCount, duplicateCount) = CountAndMakeWordLemmaTranspositions(lhLemmas, rhLemmas);
        UNIT_ASSERT_VALUES_EQUAL(transpositionCount, 1);
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[0].GetWord(), u"mp3");
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[1].GetWord(), u"скачать");
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[2].GetWord(), u"музыка");
    }

    Y_UNIT_TEST(IgnoreNonDuplicateWordsOneTransposition) {
        auto lhWords = SplitIntoTokens(u"x a y b z c", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(u"b i c j a k", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, LemmaCache, TLangMask(LANG_RUS, LANG_ENG));
        size_t transpositionCount = 0;
        size_t duplicateCount = 0;
        std::tie(transpositionCount, duplicateCount) = CountAndMakeWordLemmaTranspositions(lhLemmas, rhLemmas);
        UNIT_ASSERT_VALUES_EQUAL(transpositionCount, 1);
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[0].GetWord(), u"x");
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[1].GetWord(), u"y");
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[2].GetWord(), u"b");
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[3].GetWord(), u"z");
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[4].GetWord(), u"c");
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[5].GetWord(), u"a");
    }

    Y_UNIT_TEST(IgnoreNonDuplicateWordsTwoTranspositions) {
        auto lhWords = SplitIntoTokens(u"x a y b z c", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(u"c i b j a k", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, LemmaCache, TLangMask(LANG_RUS, LANG_ENG));
        size_t transpositionCount = 0;
        size_t duplicateCount = 0;
        std::tie(transpositionCount, duplicateCount) = CountAndMakeWordLemmaTranspositions(lhLemmas, rhLemmas);
        UNIT_ASSERT_VALUES_EQUAL(transpositionCount, 2);
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[0].GetWord(), u"x");
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[1].GetWord(), u"y");
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[2].GetWord(), u"z");
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[3].GetWord(), u"c");
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[4].GetWord(), u"b");
        UNIT_ASSERT_VALUES_EQUAL(lhLemmas[5].GetWord(), u"a");
    }

}
