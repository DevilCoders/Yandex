#pragma once

#include "calculator_data.h"

#include <kernel/dssm_applier/nn_applier/lib/layers.h>
#include <kernel/facts/common/normalize_text.h>
#include <kernel/facts/common_features/query_tokens.h>
#include <kernel/facts/dist_between_words/calc_factors.h>
#include <kernel/facts/factors_info/factor_names.h>
#include <kernel/facts/features_calculator/analyzed_word/analyzed_word.h>
#include <kernel/facts/features_calculator/embeddings/embeddings.h>

#include <library/cpp/tokenizer/split.h>
#include <ml/neocortex/neocortex_lib/helpers.h>

namespace NUnstructuredFeatures
{
    const size_t RU_FACT_SNIPPET_DSSM_EMBED_SIZE = 300;

    const TVector<TString> DSSM_ANNOTATIONS = {"query", "answer"};
    const TVector<TString> DSSM_QUERY_FEATURES = {"query_features"};

    const TVector<TString> DSSM_QUERY_EMBEDDING = {"query_embedding"};
    const TVector<TString> DSSM_DOC_EMBEDDING = {"doc_embedding"};

    const TVector<TString> ALIASES_BDSSM_ANNOTATIONS = {"alias", "query"};

    class TQueryCalculator {
    public:
        TQueryCalculator(const TCalculatorData& data,
                         const TString& query)
            : Data(data)
            , WideQuery(NormalizeText(UTF8ToWide(query)))
            , DistBetweenWordsFactors(data.WordsDistTrie, data.BigramsDistTrie)
        {
            Query = WideToUTF8(WideQuery);
            InitNormalizedMembers();
            AnalyzeQueryWords();
        }

        TQueryCalculator(const TCalculatorData& data,
                         const TString& query,
                         const TUtf16String& normedQuery)
            : Data(data)
            , WideQuery(NormalizeText(UTF8ToWide(query)))
            , NormedQueryWords(SplitIntoTokens(normedQuery, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT)))
            , DistBetweenWordsFactors(data.WordsDistTrie, data.BigramsDistTrie)
        {
            Query = WideToUTF8(WideQuery);
            AnalyzeQueryWords();
        }

        TQueryCalculator(const TCalculatorData& data,
                         const TString& query,
                         const TUtf16String& normedQuery,
                         TVector<float>&& queryEmbed)
            : TQueryCalculator(data, query, normedQuery)
        {
            RuFactSnippetDssmQueryEmbed.swap(queryEmbed);
        }

        const TVector<float>& GetQueryEmbedding() const {
            return RuFactSnippetDssmQueryEmbed;
        }

        void BuildDssmFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const;
        void BuildNumberInQueryFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const;
        void BuildQueriesLengthsFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const;
        void BuildQuestionsFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const;

        void BuildNeocortexSimilarityFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const;
        void BuildNeocortexAliasFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const;
        void BuildNeocortexFactSnipFeatures(const TQueryCalculator& rhs, const TUtf16String& answer, TFactFactorStorage& features)  const;
        void BuildNeocortexSerpItemFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const;

        static void BuildEditDistanceFeatures(const TUtf16String& lhs, const TUtf16String& rhs,
                                              const TVector<TUtf16String>& lhsWords, const TVector<TUtf16String>& rhsWords,
                                              TFactFactorStorage& features);
        static void BuildEditDistanceFeatures(const TUtf16String& lhs, const TUtf16String& rhs, TFactFactorStorage& features) {
            BuildEditDistanceFeatures(lhs, rhs,
                                      SplitIntoTokens(lhs, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT)),
                                      SplitIntoTokens(rhs, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT)),
                                      features);
        }
        void BuildEditDistanceFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const {
            BuildEditDistanceFeatures(WideQuery, rhs.WideQuery, QueryWords, rhs.QueryWords, features);
        }

        static void BuildWeightedEditDistanceFeatures(const TVector<TUtf16String>& lhsWords, const TVector<TUtf16String>& rhsWords,
                                                      TFactFactorStorage& features);
        static void BuildWeightedEditDistanceFeatures(const TUtf16String& lhs, const TUtf16String& rhs, TFactFactorStorage& features) {
            BuildWeightedEditDistanceFeatures(SplitIntoTokens(lhs, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT)),
                                              SplitIntoTokens(rhs, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT)),
                                              features);
        }
        void BuildWeightedEditDistanceFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const {
            BuildWeightedEditDistanceFeatures(QueryWords, rhs.QueryWords, features);
        }

        int GetQueryQuestionId() const;
        void BuildDistBetweenWordsFactors(const TQueryCalculator& rhs, TFactFactorStorage& features) const {
            DistBetweenWordsFactors.Calculate(QueryWords, rhs.QueryWords, features);
            DistBetweenWordsFactors.CalculateNormalized(NormedQueryWords, rhs.NormedQueryWords, features);
            DistBetweenWordsFactors.CalculateBigrams(QueryWords, rhs.QueryWords, features);
        }
        void BuildWordEmbeddingFeatures(const TQueryCalculator& rhs, const TUtf16String& answer, TFactFactorStorage& features) const;
        void BuildWordFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const;

        const TCalculatorData& GetData() const {
            return Data;
        }

        const TEmbedding& GetTextToHostsEmbed() const {
            EnsureNeocortexEmbeds();
            return TextToHostsEmbed;
        }

        void BuildCommonFeatures(const TQueryCalculator& rhs, const TUtf16String& answer, TFactFactorStorage& features) const;
        void BuildUnigramModelFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const;
        void BuildCrossModelFeatures(const TQueryCalculator& rhs, const TUtf16String& answer, TFactFactorStorage& features) const;

    private:
        void InitNormalizedMembers();
        void AnalyzeQueryWords();
        bool TryBuildRuFactSnippetDssmQueryEmbed() const;
        bool TryBuildTomatoDssmQueryEmbed() const;
        bool TryBuildAliasesBdssmQueryEmbed(bool isRhs) const;
        void EnsureNeocortexEmbeds() const;

        float GetFrequency(const TWtringBuf& word, float defaultFrequency = FACTQUERY_WORD_MEAN_FREQUENCY) const;

    private:
        static constexpr float FACTQUERY_WORD_MEAN_FREQUENCY = 0.00067487f;
        const TCalculatorData &Data;

        TUtf16String WideQuery;
        TString Query;

        // TODO: Remove it and use TypedQueryTokens everywhere.
        TVector<TUtf16String> QueryWords;
        TVector<TAnalyzedWord> AnalyzedQueryWords;
        TVector<NFacts::TTypedLemmedToken> TypedQueryTokens;

        TVector<TUtf16String> NormedQueryWords;

        mutable TVector<float> RuFactSnippetDssmQueryEmbed;
        mutable TVector<float> TomatoDssmQueryEmbed;
        mutable TVector<float> AliasesBdssmQueryEmbed;

        mutable TEmbedding TextToHostsEmbed, TextToTextEmbed;
        NDistBetweenWords::TCalcFactors DistBetweenWordsFactors;

    };
}
