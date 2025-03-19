#include "weighted_levenstein.h"

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

    float GetDeletionWeight(const TUtf16String& word, TLemmaMap& lemmaCache, const TLangMask& langs) {
        auto lhWords = SplitIntoTokens(word, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(u"", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, lemmaCache, langs);
        return CalculateWeightedLevensteinDistance(lhLemmas, rhLemmas).first;
    }

    float GetInsertionWeight(const TUtf16String& word, TLemmaMap& lemmaCache, const TLangMask& langs) {
        auto lhWords = SplitIntoTokens(u"", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(word, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, lemmaCache, langs);
        return CalculateWeightedLevensteinDistance(lhLemmas, rhLemmas).first;
    }

    float GetReplaceWeight(const TUtf16String& lhWord, const TUtf16String& rhWord, TLemmaMap& lemmaCache, const TLangMask& langs) {
        auto lhWords = SplitIntoTokens(lhWord, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(rhWord, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, lemmaCache, langs);
        return CalculateWeightedLevensteinDistance(lhLemmas, rhLemmas).first;
    }
}

Y_UNIT_TEST_SUITE(WeightedLevensteinTests) {
    TLemmaMap LemmaCache;

    Y_UNIT_TEST(CheckDeletionWeights) {
        const auto langs = TLangMask(LANG_RUS, LANG_ENG);
        UNIT_ASSERT_VALUES_EQUAL(GetDeletionWeight(u"белый", LemmaCache, langs), GetPosDeleteInsertWeight(gAdjective));
        UNIT_ASSERT_VALUES_EQUAL(GetDeletionWeight(u"быстро", LemmaCache, langs), GetPosDeleteInsertWeight(gAdverb));
        UNIT_ASSERT_VALUES_EQUAL(GetDeletionWeight(u"зато", LemmaCache, langs), GetPosDeleteInsertWeight(gConjunction));
        UNIT_ASSERT_VALUES_EQUAL(GetDeletionWeight(u"ах", LemmaCache, langs), GetPosDeleteInsertWeight(gInterjunction));
        UNIT_ASSERT_VALUES_EQUAL(GetDeletionWeight(u"девять", LemmaCache, langs), GetPosDeleteInsertWeight(gNumeral));
        UNIT_ASSERT_VALUES_EQUAL(GetDeletionWeight(u"бы", LemmaCache, langs), GetPosDeleteInsertWeight(gParticle));
        UNIT_ASSERT_VALUES_EQUAL(GetDeletionWeight(u"от", LemmaCache, langs), GetPosDeleteInsertWeight(gPreposition));
        UNIT_ASSERT_VALUES_EQUAL(GetDeletionWeight(u"стол", LemmaCache, langs), GetPosDeleteInsertWeight(gSubstantive));
        UNIT_ASSERT_VALUES_EQUAL(GetDeletionWeight(u"летит", LemmaCache, langs), GetPosDeleteInsertWeight(gVerb));
        UNIT_ASSERT_VALUES_EQUAL(GetDeletionWeight(u"второй", LemmaCache, langs), GetPosDeleteInsertWeight(gAdjNumeral));
        UNIT_ASSERT_VALUES_EQUAL(GetDeletionWeight(u"мой", LemmaCache, langs), GetPosDeleteInsertWeight(gAdjPronoun));
        UNIT_ASSERT_VALUES_EQUAL(GetDeletionWeight(u"никогда", LemmaCache, langs), GetPosDeleteInsertWeight(gAdvPronoun));
        UNIT_ASSERT_VALUES_EQUAL(GetDeletionWeight(u"он", LemmaCache, langs), GetPosDeleteInsertWeight(gSubstPronoun));
        UNIT_ASSERT_VALUES_EQUAL(GetDeletionWeight(u"the", LemmaCache, langs), GetPosDeleteInsertWeight(gArticle));
        UNIT_ASSERT_VALUES_EQUAL(GetDeletionWeight(u"бегущий", LemmaCache, langs), GetPosDeleteInsertWeight(gParticiple));
    }

    Y_UNIT_TEST(CheckInsertionWeights) {
        const auto langs = TLangMask(LANG_RUS, LANG_ENG);
        UNIT_ASSERT_VALUES_EQUAL(GetInsertionWeight(u"белый", LemmaCache, langs), GetPosDeleteInsertWeight(gAdjective));
        UNIT_ASSERT_VALUES_EQUAL(GetInsertionWeight(u"быстро", LemmaCache, langs), GetPosDeleteInsertWeight(gAdverb));
        UNIT_ASSERT_VALUES_EQUAL(GetInsertionWeight(u"зато", LemmaCache, langs), GetPosDeleteInsertWeight(gConjunction));
        UNIT_ASSERT_VALUES_EQUAL(GetInsertionWeight(u"ах", LemmaCache, langs), GetPosDeleteInsertWeight(gInterjunction));
        UNIT_ASSERT_VALUES_EQUAL(GetInsertionWeight(u"девять", LemmaCache, langs), GetPosDeleteInsertWeight(gNumeral));
        UNIT_ASSERT_VALUES_EQUAL(GetInsertionWeight(u"бы", LemmaCache, langs), GetPosDeleteInsertWeight(gParticle));
        UNIT_ASSERT_VALUES_EQUAL(GetInsertionWeight(u"от", LemmaCache, langs), GetPosDeleteInsertWeight(gPreposition));
        UNIT_ASSERT_VALUES_EQUAL(GetInsertionWeight(u"стол", LemmaCache, langs), GetPosDeleteInsertWeight(gSubstantive));
        UNIT_ASSERT_VALUES_EQUAL(GetInsertionWeight(u"летит", LemmaCache, langs), GetPosDeleteInsertWeight(gVerb));
        UNIT_ASSERT_VALUES_EQUAL(GetInsertionWeight(u"второй", LemmaCache, langs), GetPosDeleteInsertWeight(gAdjNumeral));
        UNIT_ASSERT_VALUES_EQUAL(GetInsertionWeight(u"мой", LemmaCache, langs), GetPosDeleteInsertWeight(gAdjPronoun));
        UNIT_ASSERT_VALUES_EQUAL(GetInsertionWeight(u"никогда", LemmaCache, langs), GetPosDeleteInsertWeight(gAdvPronoun));
        UNIT_ASSERT_VALUES_EQUAL(GetInsertionWeight(u"он", LemmaCache, langs), GetPosDeleteInsertWeight(gSubstPronoun));
        UNIT_ASSERT_VALUES_EQUAL(GetInsertionWeight(u"the", LemmaCache, langs), GetPosDeleteInsertWeight(gArticle));
        UNIT_ASSERT_VALUES_EQUAL(GetInsertionWeight(u"бегущий", LemmaCache, langs), GetPosDeleteInsertWeight(gParticiple));
    }

    Y_UNIT_TEST(CheckReplaceSamePosWeight) {
        const auto langs = TLangMask(LANG_RUS, LANG_ENG);
        UNIT_ASSERT_VALUES_EQUAL(GetReplaceWeight(u"белый", u"зелёный", LemmaCache, langs),
            GetPosDeleteInsertWeight(gAdjective));
    }

    Y_UNIT_TEST(CheckReplaceDiffPosWeight) {
        const auto langs = TLangMask(LANG_RUS, LANG_ENG);
        UNIT_ASSERT_VALUES_EQUAL(GetReplaceWeight(u"белый", u"никогда", LemmaCache, langs),
            Max(GetPosDeleteInsertWeight(gAdjective), GetPosDeleteInsertWeight(gAdvPronoun)));
    }

    Y_UNIT_TEST(CheckEditDistance) {
        const auto langs = TLangMask(LANG_RUS, LANG_ENG);
        auto lhWords = SplitIntoTokens(u"белый и пушистый заяц", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(u"серый и пушистый заяц", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, LemmaCache, langs);
        float distance = 0.0f;
        float chainLen = 0.0f;
        std::tie(distance, chainLen) = CalculateWeightedLevensteinDistance(lhLemmas, rhLemmas);
        UNIT_ASSERT_VALUES_EQUAL(distance, GetPosDeleteInsertWeight(gAdjective));
        UNIT_ASSERT_VALUES_EQUAL(chainLen, GetPosDeleteInsertWeight(gConjunction) + GetPosDeleteInsertWeight(gAdjective)
            + GetPosDeleteInsertWeight(gSubstantive) + GetPosDeleteInsertWeight(gAdjective));
    }

    Y_UNIT_TEST(CheckFormChangeEditDistance) {
        const auto langs = TLangMask(LANG_RUS, LANG_ENG);
        auto lhWords = SplitIntoTokens(u"белый и пушистый заяц", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        auto rhWords = SplitIntoTokens(u"белого и пушистого зайца", TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TWordLemmaPairVector lhLemmas;
        TWordLemmaPairVector rhLemmas;
        std::tie(lhLemmas, rhLemmas) = MakeWordLemmaPtrArrays(lhWords, rhWords, LemmaCache, langs);
        float distance = 0.0f;
        float chainLen = 0.0f;
        std::tie(distance, chainLen) = CalculateWeightedLevensteinDistance(lhLemmas, rhLemmas);
        UNIT_ASSERT_VALUES_EQUAL(distance, 3 * GetFormChangeWeight());
        UNIT_ASSERT_VALUES_EQUAL(chainLen, GetPosDeleteInsertWeight(gAdjective) + GetPosDeleteInsertWeight(gConjunction)
            + GetPosDeleteInsertWeight(gAdjective) + GetPosDeleteInsertWeight(gSubstantive));
    }
}

