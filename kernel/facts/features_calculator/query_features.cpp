#include "query_features.h"

#include <kernel/dssm_applier/nn_applier/lib/layers.h>
#include <kernel/facts/common_features/query_model.h>
#include <kernel/facts/common_features/cross_model.h>
#include <kernel/facts/dssm_applier/prepare_for_dssm.h>
#include <kernel/facts/edit_distance_features/weighted_levenstein.h>
#include <kernel/facts/edit_distance_features/basic_levenstein.h>
#include <kernel/facts/edit_distance_features/transpose_words.h>
#include <kernel/facts/edit_distance_features/word_lemma_pair.h>
#include <kernel/facts/word_difference/word_difference.h>
#include <kernel/normalize_by_lemmas/normalize.h>
#include <library/cpp/dot_product/dot_product.h>

#include <util/charset/unidata.h>
#include <util/charset/utf8.h>
#include <util/generic/algorithm.h>

namespace NUnstructuredFeatures
{
    const size_t TOMATO_DSSM_EMBED_SIZE = 128;
    const size_t ALIASES_BDSSM_EMBED_SIZE = 128;
    const THashMap<TUtf16String, int> QUESTION_WORDS = THashMap<TUtf16String, int>{ {u"когда", 0},
                                                                               {u"кто", 1},
                                                                               {u"что", 2},
                                                                               {u"где", 3},
                                                                               {u"куда", 4},
                                                                               {u"откуда", 5},
                                                                               {u"ком", 6},
                                                                               {u"кому", 7},
                                                                               {u"чем", 8},
                                                                               {u"чему", 9},
                                                                               {u"чём", 10},
                                                                               {u"сколько", 11},
                                                                               {u"как", 12},
                                                                               {u"почему", 13},
                                                                               {u"должен", 14},
                                                                               {u"нужен", 15}};


    template <typename T>
    float NormalizePositiveNumber(T num) {
        return float(num) / (float(num) + 1.0);
    }

    bool HasDigit(const TUtf16String& word) {
        for (wchar16 letter : word) {
            if (IsDigit(letter)) {
                return true;
            }
        }
        return false;
    }

    float TQueryCalculator::GetFrequency(const TWtringBuf& word, float defaultFrequency) const {
        ENSURE_DATA(Data, Pure);
        TPureBase::TPureData pureData = Data.Pure->GetByForm(word.data(), word.size(), NPure::AllCase, LANG_RUS);
        float frequency = pureData.GetFreq() == 0 ? defaultFrequency
                                                  : static_cast<float>(pureData.GetFreq()) / Data.Pure->GetCollectionLength();
        return frequency;
    }

    void TQueryCalculator::BuildEditDistanceFeatures(const TUtf16String& lhs, const TUtf16String& rhs,
                                                     const TVector<TUtf16String>& lhsWords, const TVector<TUtf16String>& rhsWords,
                                                     TFactFactorStorage& features) {
        using namespace NEditDistanceFeatures;

        float symbolEditDistance = 0;
        size_t symbolEditChainLen = 0;
        std::tie(symbolEditDistance, symbolEditChainLen) = GetLevensteinDistance(lhs, rhs);

        features[FI_SYMBOL_EDIT_DISTANCE] = (symbolEditChainLen > 0 ? symbolEditDistance / static_cast<float>(symbolEditChainLen) : 0.0f);

        float wordEditDistance = 0;
        size_t wordEditChainLen = 0;
        std::tie(wordEditDistance, wordEditChainLen) = GetLevensteinDistance(lhsWords, rhsWords);
        features[FI_WORD_EDIT_DISTANCE] = (wordEditChainLen > 0 ? wordEditDistance / static_cast<float>(wordEditChainLen) : 0.0f);

        // TODO: Make optional
        // The factor is deprecated and isn't used in existing formulas
        TVector<TUtf16String> lhsSortedWords(lhsWords);
        Sort(lhsSortedWords);
        TVector<TUtf16String> rhsSortedWords(rhsWords);
        Sort(rhsSortedWords);
        float wordSortedEditDistance = 0;
        size_t wordSortedEditChainLen = 0;
        std::tie(wordSortedEditDistance, wordSortedEditChainLen) = GetLevensteinDistance(lhsSortedWords, rhsSortedWords);
        features[FI_SORTED_WORD_EDIT_DISTANCE] = (wordSortedEditChainLen > 0 ? wordSortedEditDistance / static_cast<float>(wordSortedEditChainLen) : 0.0f);

        float nearestSubsetDistance = 0.0f;
        for (const TUtf16String& lhsWord : lhsWords) {
            float nearestDistance = 1.0f;
            for (const TUtf16String& rhsWord : rhsWords) {
                float distance;
                size_t chainLen;
                std::tie(distance, chainLen) = NEditDistanceFeatures::GetLevensteinDistance(lhsWord, rhsWord);
                nearestDistance = Min(nearestDistance, chainLen > 0 ? distance / chainLen : 0.0f);
            }
            nearestSubsetDistance += nearestDistance;
        }
        features[FI_NEAREST_WORD_SUBSET_SYMBOL_EDIT_DISTANCE] = lhsWords.empty() ? 1.0f : nearestSubsetDistance / lhsWords.size();
    }

    void TQueryCalculator::BuildWeightedEditDistanceFeatures(const TVector<TUtf16String>& lhsWords, const TVector<TUtf16String>& rhsWords,
                                                             TFactFactorStorage& features) {
        using namespace NEditDistanceFeatures;

        // TODO: FACTS-1871 - make lemma cache global
        NEditDistanceFeatures::TWordLemmaPair::TLemmaArrayMap lemmaCache;
        auto wordToWordLemma = [&lemmaCache](const TUtf16String& word) {
            return TWordLemmaPair::GetCachedWordLemmaPair(word, lemmaCache, TLangMask(LANG_RUS, LANG_ENG));
        };
        TVector<TWordLemmaPair> lhsWordLemmas;
        lhsWordLemmas.reserve(lhsWords.size());
        Transform(lhsWords.begin(), lhsWords.end(), std::back_inserter(lhsWordLemmas), wordToWordLemma);
        TVector<TWordLemmaPair> rhsWordLemmas;
        lhsWordLemmas.reserve(rhsWords.size());
        Transform(rhsWords.begin(), rhsWords.end(), std::back_inserter(rhsWordLemmas), wordToWordLemma);

        size_t transposeCount = 0;
        size_t duplicateCount = 0;
        std::tie(transposeCount, duplicateCount) = CountAndMakeWordLemmaTranspositions(lhsWordLemmas, rhsWordLemmas);
        features[FI_LEMMA_DUPLICATE_COUNT] = static_cast<float>(duplicateCount);

        float weightedLevensteinDistance = 0;
        float weightedEditChainLen = 0;
        std::tie(weightedLevensteinDistance, weightedEditChainLen) = CalculateWeightedLevensteinDistance(lhsWordLemmas, rhsWordLemmas);
        features[FI_LEMMA_WEIGHTED_EDIT_DISTANCE] = weightedLevensteinDistance;
        features[FI_LEMMA_WEIGHTED_EDIT_CHAIN_LEN] = weightedEditChainLen;
    }

    void TQueryCalculator::InitNormalizedMembers() {
        ENSURE_DATA(Data, Normalizer);
        TUtf16String normedQuery = NormalizeByLemmas(WideQuery, *Data.Normalizer);
        NormedQueryWords = SplitIntoTokens(normedQuery, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
    }

    void TQueryCalculator::AnalyzeQueryWords() {
        QueryWords = SplitIntoTokens(WideQuery, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));

        AnalyzedQueryWords.reserve(QueryWords.size());
        for (const TUtf16String& word : QueryWords) {
            TWLemmaArray foundLemmas;
            NLemmer::AnalyzeWord(word.data(), word.size(), foundLemmas, TLangMask(LANG_RUS, LANG_ENG));
            AnalyzedQueryWords.push_back({word, std::move(foundLemmas), GetFrequency(word)});
        }

        TTokenizerSplitParams tokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT);
        NFacts::TTypedLemmedTokenHandler queryHandler(&TypedQueryTokens, tokenizerSplitParams);
        TNlpTokenizer queryTokenizer(queryHandler);
        queryTokenizer.Tokenize(WideQuery);
    }

    void TQueryCalculator::BuildNumberInQueryFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const {
        TVector<TUtf16String> digitWords1, digitWords2, intersection;
        for (const TUtf16String& word : rhs.NormedQueryWords) {
            if (HasDigit(word)) {
                digitWords1.push_back(word);
            }
        }

        for (const TUtf16String& word : NormedQueryWords) {
            if (HasDigit(word)) {
                digitWords2.push_back(word);
            }
        }

        float result = 0.0;

        if (digitWords1.size() == digitWords2.size()) {
            std::sort(digitWords1.begin(), digitWords1.end());
            std::sort(digitWords2.begin(), digitWords2.end());
            std::set_intersection(digitWords1.begin(), digitWords1.end(), digitWords2.begin(), digitWords2.end(), back_inserter(intersection));
            if (intersection.size() == digitWords1.size()) {
                result = 1.0;
            }

        }
        features[FI_QUERY_CONTAINS_ALL_RHS_NUMBERS] = result;
    }

    bool TQueryCalculator::TryBuildRuFactSnippetDssmQueryEmbed() const {
        if (Data.RuFactSnippetDssmModel == nullptr) {
            return false;
        }
        if (RuFactSnippetDssmQueryEmbed.empty()) {
            NNeuralNetApplier::TSample* sample = new NNeuralNetApplier::TSample(DSSM_ANNOTATIONS, { Query, TString("") });
            Data.RuFactSnippetDssmModel->Apply(sample, DSSM_QUERY_FEATURES, RuFactSnippetDssmQueryEmbed);
        }
        return true;
    }

    bool TQueryCalculator::TryBuildTomatoDssmQueryEmbed() const {
        if (Data.TomatoDssmModel == nullptr) {
            return false;
        }
        if (TomatoDssmQueryEmbed.empty()) {
            NNeuralNetApplier::TSample* sample = new NNeuralNetApplier::TSample(DSSM_ANNOTATIONS, { Query, TString("") });
            Data.TomatoDssmModel->Apply(sample, DSSM_QUERY_EMBEDDING, TomatoDssmQueryEmbed);
        }
        return true;
    }

    bool TQueryCalculator::TryBuildAliasesBdssmQueryEmbed(bool isRhs) const {
        if (Data.AliasesBdssmModel == nullptr) {
            return false;
        }
        if (AliasesBdssmQueryEmbed.empty()) {
            if (isRhs) {
                NNeuralNetApplier::TSample* sample = new NNeuralNetApplier::TSample(ALIASES_BDSSM_ANNOTATIONS, { TString(""), Query });
                Data.AliasesBdssmModel->Apply(sample, DSSM_DOC_EMBEDDING, AliasesBdssmQueryEmbed);
            } else {
                NNeuralNetApplier::TSample* sample = new NNeuralNetApplier::TSample(ALIASES_BDSSM_ANNOTATIONS, { Query, TString("") });
                Data.AliasesBdssmModel->Apply(sample, DSSM_QUERY_EMBEDDING, AliasesBdssmQueryEmbed);
            }
        }
        return true;
    }

    void TQueryCalculator::BuildDssmFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const {
        if (TryBuildRuFactSnippetDssmQueryEmbed() && rhs.TryBuildRuFactSnippetDssmQueryEmbed()) {
            features[FI_RU_FACT_SNIPPET_DSSM_QUERY_CANDIDATE_SCORE] = DotProduct(RuFactSnippetDssmQueryEmbed.begin(), rhs.RuFactSnippetDssmQueryEmbed.begin(), RU_FACT_SNIPPET_DSSM_EMBED_SIZE);
        }
        if (TryBuildTomatoDssmQueryEmbed() && rhs.TryBuildTomatoDssmQueryEmbed()) {
            features[FI_TOMATO_DSSM_QUERY_CANDIDATE_SCORE] = DotProduct(TomatoDssmQueryEmbed.begin(), rhs.TomatoDssmQueryEmbed.begin(), TOMATO_DSSM_EMBED_SIZE);
        }
        if (TryBuildAliasesBdssmQueryEmbed(false) && rhs.TryBuildAliasesBdssmQueryEmbed(true)) {
            features[FI_ALIASES_BDSSM_SCORE] = DotProduct(AliasesBdssmQueryEmbed.begin(), rhs.AliasesBdssmQueryEmbed.begin(), ALIASES_BDSSM_EMBED_SIZE);
        }
    }

    void TQueryCalculator::BuildNeocortexSimilarityFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const {
        EnsureNeocortexEmbeds();
        rhs.EnsureNeocortexEmbeds();
        features[FI_NEOCORTEX_TTT_SIMILARITY] = Cosine(TextToTextEmbed, rhs.TextToTextEmbed);
        features[FI_NEOCORTEX_TTH_SIMILARITY] = Cosine(TextToHostsEmbed, rhs.TextToHostsEmbed);
    }

    void TQueryCalculator::EnsureNeocortexEmbeds() const {
        ENSURE_DATA(Data, TextToTextNeocortexModel);
        ENSURE_DATA(Data, TextToHostsNeocortexModel);
        if (TextToTextEmbed.Empty())
            TextToTextEmbed.Assign(NNeocortex::PrepareEmbedding(*Data.TextToTextNeocortexModel, Query, true));
        if (TextToHostsEmbed.Empty())
            TextToHostsEmbed.Assign(NNeocortex::PrepareEmbedding(*Data.TextToHostsNeocortexModel, Query, true));
    }

    void TQueryCalculator::BuildQueriesLengthsFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const {
        features[FI_QUERY_LEN] = static_cast<float>(WideQuery.size());
        features[FI_RHS_LEN] = static_cast<float>(rhs.WideQuery.size());
    }

    int GetQuestionId(const TVector<TUtf16String>& words) {
        for(const TUtf16String& word : words) {
            if (QUESTION_WORDS.contains(word)) {
                return QUESTION_WORDS.at(word);
            }
        }
        return -1;
    }

    int TQueryCalculator::GetQueryQuestionId() const {
        return GetQuestionId(QueryWords);
    }

    void TQueryCalculator::BuildQuestionsFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const {
        int queryQuestionId = GetQuestionId(QueryWords);
        int aliasQuestionId = GetQuestionId(rhs.QueryWords);
        features[FI_SAME_QUESTION_WORD] = (queryQuestionId == aliasQuestionId);
    }

    void TQueryCalculator::BuildNeocortexAliasFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const {
        ENSURE_DATA(Data, LemmaAliasNeocortexModel);

        TUtf16String lemmaDiffLeft, lemmaDiffRight;
        CalcLemmaDifferences(QueryWords, rhs.QueryWords, lemmaDiffLeft, lemmaDiffRight);

        TVector<TUtf16String> rareWords;
        NNeocortex::ITextClassifier::TContext context1(Data.LemmaAliasNeocortexModel->V, WideToUTF8(lemmaDiffLeft), &rareWords);
        NNeocortex::ITextClassifier::TContext context2(Data.LemmaAliasNeocortexModel->V, WideToUTF8(lemmaDiffRight), &rareWords);
        Data.LemmaAliasNeocortexModel->Classifier->Prepare(context1, true);
        Data.LemmaAliasNeocortexModel->Classifier->Prepare(context2, false);

        features[FI_NEOCORTEX_ALIAS_LEMMA_COSINE] = Cosine(context1.Vec, context2.Vec);
    }

    void TQueryCalculator::BuildNeocortexFactSnipFeatures(const TQueryCalculator& rhs, const TUtf16String& answer, TFactFactorStorage& features) const {
        const NNeocortex::TTextClassifierPack* model = Data.TextToTextFactSnipNeocortexModel.Get();
        TVector<TUtf16String> rareWords1, rareWords2, rareWords3;
        NNeocortex::ITextClassifier::TContext contextQ1(model->V, Query, &rareWords1);
        NNeocortex::ITextClassifier::TContext contextQ2(model->V, rhs.Query, &rareWords2);
        NNeocortex::ITextClassifier::TContext contextF(model->V, WideToUTF8(answer), &rareWords3);
        model->Classifier->Prepare(contextQ1, true);
        model->Classifier->Prepare(contextQ2, true);
        model->Classifier->Prepare(contextF, false);
        TEmbedding embQ1(contextQ1.Vec);
        TEmbedding embQ2(contextQ2.Vec);
        TEmbedding embF(contextF.Vec);

        float score1 = Cosine(embQ1, embF);
        float score2 = Cosine(embQ2, embF);
        float score3 = Cosine(embQ1, embQ2);

        features[FI_FACT_SNIP_NEOCORTEX_QUERY_ANSWER_COSINE] = score1;
        features[FI_FACT_SNIP_NEOCORTEX_QUERY_ANSWER_COSINE_MINUS_RHS_ANSWER_COSINE] = score1 - score2;
        features[FI_FACT_SNIP_NEOCORTEX_QUERY_RHS_COSINE] = score3;
    }

    void TQueryCalculator::BuildWordEmbeddingFeatures(const TQueryCalculator& rhs, const TUtf16String& answer, TFactFactorStorage& features) const {
        THashMap<TString, TVector<TAnalyzedWord>> input;
        input["query1"] = rhs.AnalyzedQueryWords;
        input["query2"] = AnalyzedQueryWords;

        TVector<TUtf16String> answerWords = SplitIntoTokens(answer, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TVector<TAnalyzedWord>& analyzedAnswerWords = input["answer"];
        analyzedAnswerWords.reserve(answerWords.size());

        for (TUtf16String& word : answerWords) {
            float frequency = GetFrequency(word);
            analyzedAnswerWords.push_back({std::move(word), {}, frequency});
        }
        Data.WordEmbeddingFeaturesBuilder->BuildFeatures(input, features);
    }

    void TQueryCalculator::BuildWordFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const {

        auto extractFeaturesData = [this](const TVector<TAnalyzedWord>& analyzedWords,
                                      THashMap<TWtringBuf, float>& lemmaFrequencies,
                                      THashSet<TWtringBuf>& nouns) {
            for (const TAnalyzedWord& analyzedWord : analyzedWords) {
                for (const TYandexLemma& yLemma : analyzedWord.Lemmas) {
                    TWtringBuf lemmaWideStr(yLemma.GetText(), yLemma.GetTextLength());
                    lemmaFrequencies[lemmaWideStr] = GetFrequency(lemmaWideStr);

                    const char* stemGram = yLemma.GetStemGram();
                    for (size_t i = 0; stemGram[i]; i++) {
                        if (NTGrammarProcessing::ch2tg(stemGram[i]) == gSubstantive) {
                            nouns.insert(lemmaWideStr);
                            break;
                        }
                    }
                }
            }
        };
        THashMap<TWtringBuf, float> lhsLemmaFrequencies, rhsLemmaFrequencies;
        THashSet<TWtringBuf> lhsNouns, rhsNouns;

        extractFeaturesData(AnalyzedQueryWords, lhsLemmaFrequencies, lhsNouns);
        extractFeaturesData(rhs.AnalyzedQueryWords, rhsLemmaFrequencies, rhsNouns);

        float lhsDiffMinFrequency = 1.0f;
        float rhsDiffMinFrequency = 1.0f;
        float commonNounMinFrequency = 1.0f;

        for (const auto lemmaFreq : lhsLemmaFrequencies) {
            if (!rhsLemmaFrequencies.contains(lemmaFreq.first)) {
                lhsDiffMinFrequency = Min(lhsDiffMinFrequency, lemmaFreq.second);
            }
        }

        for (const auto lemmaFreq : rhsLemmaFrequencies) {
            if (!lhsLemmaFrequencies.contains(lemmaFreq.first)) {
                rhsDiffMinFrequency = Min(rhsDiffMinFrequency, lemmaFreq.second);
            } else {
                if (lhsNouns.contains(lemmaFreq.first)) {
                    commonNounMinFrequency = Min(commonNounMinFrequency, lemmaFreq.second);
                }
            }
        }

        features[FI_LHS_DIFF_MIN_LEMMA_FREQUENCY] = lhsDiffMinFrequency;
        features[FI_RHS_DIFF_MIN_LEMMA_FREQUENCY] = rhsDiffMinFrequency;
        features[FI_COMMON_NOUN_MIN_FREQUENCY] = commonNounMinFrequency;
    }

    void TQueryCalculator::BuildNeocortexSerpItemFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const {
        ENSURE_DATA(Data, SerpItemsNeocortexModel);

        TVector<float> lhsEmbedding = PrepareEmbedding(*Data.SerpItemsNeocortexModel, Query, true);
        TVector<float> rhsEmbedding = PrepareEmbedding(*Data.SerpItemsNeocortexModel, rhs.Query, true);
        TVector<float> unionFactsEmbedding = PrepareEmbedding(*Data.SerpItemsNeocortexModel, "union-facts", false);
        TVector<float> positiveQueryMxEmbedding = PrepareEmbedding(*Data.SerpItemsNeocortexModel, "positive_query_mx", false);
        TVector<float> wizImagesEmbedding = PrepareEmbedding(*Data.SerpItemsNeocortexModel, "wiz-images", false);
        TVector<float> wizEntitySearchEmbedding = PrepareEmbedding(*Data.SerpItemsNeocortexModel, "wiz-entity_search", false);

        features[FI_NEOCORTEX_SERP_ITEMS_LHS_UNION_FACTS_COSINE] = Cosine(lhsEmbedding, unionFactsEmbedding);
        features[FI_NEOCORTEX_SERP_ITEMS_LHS_POSITIVE_QUERY_MX_COSINE] = Cosine(lhsEmbedding, positiveQueryMxEmbedding);
        features[FI_NEOCORTEX_SERP_ITEMS_RHS_WIZ_IMAGES_COSINE] = Cosine(rhsEmbedding, wizImagesEmbedding);
        features[FI_NEOCORTEX_SERP_ITEMS_RHS_WIZ_ENTITY_SEARCH_COSINE] = Cosine(rhsEmbedding, wizEntitySearchEmbedding);
    }

    void TQueryCalculator::BuildCommonFeatures(const TQueryCalculator& rhs, const TUtf16String& answer, TFactFactorStorage& features) const {
        BuildUnigramModelFeatures(rhs, features);
        BuildCrossModelFeatures(rhs, answer, features);
    }

    void TQueryCalculator::BuildUnigramModelFeatures(const TQueryCalculator& rhs, TFactFactorStorage& features) const {
        float left = NFacts::CalculateQueryModelFeature(TypedQueryTokens, Data.QueryUnigramModel.Get());
        float right = NFacts::CalculateQueryModelFeature(rhs.TypedQueryTokens, Data.QueryUnigramModel.Get());
        features[FI_QUERY_MODEL_DIFF] = fabs(left - right);
        features[FI_QUERY_MODEL_MIN] = Min(left, right);
        features[FI_QUERY_MODEL_MAX] = Max(left, right);
    }

    void TQueryCalculator::BuildCrossModelFeatures(const TQueryCalculator& rhs, const TUtf16String& answer, TFactFactorStorage& features) const {
        TTokenizerSplitParams tokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT);
        TVector<NFacts::TTypedLemmedToken> answerTokens;
        NFacts::TTypedLemmedTokenHandler answerHandler(&answerTokens, tokenizerSplitParams);
        TNlpTokenizer answerTokenizer(answerHandler);
        answerTokenizer.Tokenize(answer);

        float left = NFacts::CalculateCrossModelFeature(TypedQueryTokens, answerTokens, Data.CrossModel.Get());
        float right = NFacts::CalculateCrossModelFeature(rhs.TypedQueryTokens, answerTokens, Data.CrossModel.Get());

        features[FI_CROSS_MODEL_DIFF] = fabs(left - right);
        features[FI_CROSS_MODEL_MIN] = Min(left, right);
        features[FI_CROSS_MODEL_MAX] = Max(left, right);
    }

}
