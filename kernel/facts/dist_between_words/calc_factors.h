#pragma once

#include "levenstein_word_dist.h"

#include <kernel/facts/factors_info/factor_names.h>

#include <library/cpp/tokenizer/split.h>

#include <utility>

namespace NDistBetweenWords {

    class TCalcFactor {
    public:

        static TCalcFactor BuildHost5(const TWordDistTriePtr& trie) {
            return TCalcFactor(trie, [](const TTrieData& data) { return data.Host5; });
        }

        static TCalcFactor BuildHost10(const TWordDistTriePtr& trie) {
            return TCalcFactor(trie, [](const TTrieData& data) { return data.Host10; });
        }

        static TCalcFactor BuildUrl5(const TWordDistTriePtr& trie) {
            return TCalcFactor(trie, [](const TTrieData& data) { return data.Url5; });
        }

        static TCalcFactor BuildUrl10(const TWordDistTriePtr& trie) {
            return TCalcFactor(trie, [](const TTrieData& data) { return data.Url10; });
        }

        // after normalization
        float Calc(const TVector<TUtf16String>& query1, const TVector<TUtf16String>& query2, bool withNorm) const;

        // normalized queries
        float Calc(const TString& query1, const TString& query2) const;

        // not normalized
        float CalcBigram(const TVector<TUtf16String>& query1, const TVector<TUtf16String>& query2) const;
        float CalcBigram(const TString& query1, const TString& query2) const;

    private:
        TCalcFactor(const TWordDistTriePtr& trie, TTrieData::TGetter getter):
                WeightSource(trie, std::move(getter))
        {}

        TLevensteinWordDist WeightSource;
    };


    class TCalcFactors {
    public:
        TCalcFactors(const TWordDistTriePtr& trie, const TWordDistTriePtr& bigramsTrie);

        void Calculate(const TVector<TUtf16String>& lhs, const TVector<TUtf16String>& rhs, NUnstructuredFeatures::TFactFactorStorage& features) const;
        void CalculateNormalized(const TVector<TUtf16String>& lhs, const TVector<TUtf16String>& rhs, NUnstructuredFeatures::TFactFactorStorage& features) const;

        void CalculateBigrams(const TVector<TUtf16String>& lhs, const TVector<TUtf16String>& rhs, NUnstructuredFeatures::TFactFactorStorage& features) const;
    private:
        TCalcFactor WordsDistUrl5;
        TCalcFactor WordsDistUrl10;
        TCalcFactor WordsDistHost5;
        TCalcFactor WordsDistHost10;

        TCalcFactor BigramsDistUrl5;
        TCalcFactor BigramsDistUrl10;
        TCalcFactor BigramsDistHost5;
        TCalcFactor BigramsDistHost10;
    };

}
