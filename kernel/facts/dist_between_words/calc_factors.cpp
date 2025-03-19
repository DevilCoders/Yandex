#include "calc_factors.h"

#include <library/cpp/charset/wide.h>
#include <library/cpp/tokenizer/split.h>
#include <library/cpp/string_utils/levenshtein_diff/levenshtein_diff.h>

#include "levenstein_dist_with_bigrams.h"

using namespace NUnstructuredFeatures;

namespace NDistBetweenWords {
    float TCalcFactor::Calc(const TString& query1, const TString& query2) const {
        TUtf16String normQuery1 = CharToWide(query1, CODES_UTF8);
        TUtf16String normQuery2 = CharToWide(query2, CODES_UTF8);
        TVector<TUtf16String> query1Words = SplitIntoTokens(normQuery1, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TVector<TUtf16String> query2Words = SplitIntoTokens(normQuery2, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        return Calc(query1Words, query2Words, false);
    }

    float TCalcFactor::Calc(const TVector<TUtf16String>& query1, const TVector<TUtf16String>& query2, bool withNorm) const {
        float distance = 0;
        NLevenshtein::TEditChain chain;
        NLevenshtein::GetEditChain(query1,
                query2,
                chain,
                &distance,
                [this](const TUtf16String& str1, const TUtf16String& str2) -> float { return WeightSource.ReplaceWeight(str1, str2); },
                [this](const TUtf16String& str) -> float { return WeightSource.DeleteWeight(str); },
                [this](const TUtf16String& str) -> float { return WeightSource.InsertWeight(str); }
        );
        if (withNorm) {
            return distance / Max(1.0f, static_cast<float>(chain.size()));
        } else {
            return distance;
        }
    }

    float TCalcFactor::CalcBigram(const TString& query1, const TString& query2) const {
        TUtf16String normQuery1 = CharToWide(query1, CODES_UTF8);
        TUtf16String normQuery2 = CharToWide(query2, CODES_UTF8);
        TVector<TUtf16String> query1Words = SplitIntoTokens(normQuery1, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TVector<TUtf16String> query2Words = SplitIntoTokens(normQuery2, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        return CalcBigram(query1Words, query2Words);
    }

    float TCalcFactor::CalcBigram(const TVector<TUtf16String>& query1, const TVector<TUtf16String>& query2) const {
        return NLevenstein::BigramLevensteinDistance(query1,
                query2,
                TLevensteinWordDist::BIGRAM_DELIMITER,
                [this](const TUtf16String& str1, const TUtf16String& str2) -> float { return WeightSource.ReplaceWeight(str1, str2); },
                [this](const TUtf16String& str) -> float { return WeightSource.DeleteWeight(str); },
                [this](const TUtf16String& str) -> float { return WeightSource.InsertWeight(str); }
        );
    }

    TCalcFactors::TCalcFactors(const TWordDistTriePtr& trie, const TWordDistTriePtr& bigramsTrie):
            WordsDistUrl5(TCalcFactor::BuildUrl5(trie))
            , WordsDistUrl10(TCalcFactor::BuildUrl10(trie))
            , WordsDistHost5(TCalcFactor::BuildHost5(trie))
            , WordsDistHost10(TCalcFactor::BuildHost10(trie))
            , BigramsDistUrl5(TCalcFactor::BuildUrl5(bigramsTrie))
            , BigramsDistUrl10(TCalcFactor::BuildUrl10(bigramsTrie))
            , BigramsDistHost5(TCalcFactor::BuildHost5(bigramsTrie))
            , BigramsDistHost10(TCalcFactor::BuildHost10(bigramsTrie))
    {}

    void TCalcFactors::Calculate(const TVector<TUtf16String>& lhs, const TVector<TUtf16String>& rhs, TFactFactorStorage& features) const {
        features[FI_WORD_DIST_URL_5] = WordsDistUrl5.Calc(lhs, rhs, false);
        features[FI_WORD_DIST_URL_10] = WordsDistUrl10.Calc(lhs, rhs, false);
        features[FI_WORD_DIST_HOST_5] = WordsDistHost5.Calc(lhs, rhs, false);
        features[FI_WORD_DIST_HOST_10] = WordsDistHost10.Calc(lhs, rhs, false);
        features[FI_WORD_DIST_URL_5_NORM] = WordsDistUrl5.Calc(lhs, rhs, true);
        features[FI_WORD_DIST_URL_10_NORM] = WordsDistUrl10.Calc(lhs, rhs, true);
        features[FI_WORD_DIST_HOST_5_NORM] = WordsDistHost5.Calc(lhs, rhs, true);
        features[FI_WORD_DIST_HOST_10_NORM] = WordsDistHost10.Calc(lhs, rhs, true);
    }

    void TCalcFactors::CalculateNormalized(const TVector<TUtf16String>& lhs, const TVector<TUtf16String>& rhs, NUnstructuredFeatures::TFactFactorStorage& features) const {
        features[FI_WORD_DIST_URL_5_NORM_QUERY] = WordsDistUrl5.Calc(lhs, rhs, false);
        features[FI_WORD_DIST_URL_10_NORM_QUERY] = WordsDistUrl10.Calc(lhs, rhs, false);
        features[FI_WORD_DIST_HOST_5_NORM_QUERY] = WordsDistHost5.Calc(lhs, rhs, false);
        features[FI_WORD_DIST_HOST_10_NORM_QUERY] = WordsDistHost10.Calc(lhs, rhs, false);
        features[FI_WORD_DIST_URL_5_NORM_NORM_QUERY] = WordsDistUrl5.Calc(lhs, rhs, true);
        features[FI_WORD_DIST_URL_10_NORM_NORM_QUERY] = WordsDistUrl10.Calc(lhs, rhs, true);
        features[FI_WORD_DIST_HOST_5_NORM_NORM_QUERY] = WordsDistHost5.Calc(lhs, rhs, true);
        features[FI_WORD_DIST_HOST_10_NORM_NORM_QUERY] = WordsDistHost10.Calc(lhs, rhs, true);
    }

    void TCalcFactors::CalculateBigrams(const TVector<TUtf16String>& lhs, const TVector<TUtf16String>& rhs, NUnstructuredFeatures::TFactFactorStorage& features) const {
        features[FI_BIGRAM_DIST_URL_5] = BigramsDistUrl5.CalcBigram(lhs, rhs);
        features[FI_BIGRAM_DIST_URL_10] = BigramsDistUrl10.CalcBigram(lhs, rhs);
        features[FI_BIGRAM_DIST_HOST_5] = BigramsDistHost5.CalcBigram(lhs, rhs);
        features[FI_BIGRAM_DIST_HOST_10] = BigramsDistHost10.CalcBigram(lhs, rhs);

    }
}
