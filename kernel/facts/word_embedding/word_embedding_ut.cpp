#include "dictionary.h"
#include "features_calculator.h"
#include "preprocessor.h"

#include <kernel/facts/features_calculator/analyzed_word/analyzed_word.h>

#include <kernel/lemmer/core/language.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>
#include <util/stream/file.h>
#include <util/string/vector.h>

using namespace NUnstructuredFeatures;
using namespace NUnstructuredFeatures::NWordEmbedding;

namespace {
    void AssertValues(const TFactFactorStorage& a, const TVector<float>& b) {
        UNIT_ASSERT(a.Size() >= b.size());
        for (size_t i = 0; i < b.size(); i++) {
            UNIT_ASSERT_DOUBLES_EQUAL(a[i], b[i], std::numeric_limits<float>::epsilon());
        }
    }

    TVector<EFactorId> BuildFactorIds(size_t firstId, size_t len) {
        TVector<EFactorId> factorIds;
        for (size_t id = firstId; id < firstId + len; id++) {
            factorIds.push_back(static_cast<EFactorId>(id));
        }
        return factorIds;
    }

    TDictionary* MakeDictionary() {
        TString dictWords = NResource::Find("dict_words");
        TFileOutput("dict.words").Write(dictWords.data(), dictWords.size());
        TString dictVectors = NResource::Find("dict_vectors");
        TFileOutput("dict.vectors").Write(dictVectors.data(), dictVectors.size());
        return new TDictionary("dict.words", "dict.vectors");
    }

    TVector<TEmbedding> GetEmbeddings(const TVector<TAnalyzedWord>& analyzedWords, TDictionary* dictionary) {
        TVector<TEmbedding> embeddings;
        for (const TAnalyzedWord& analyzedWord : analyzedWords) {
            TEmbedding embedding;
            if (dictionary->GetEmbedding(analyzedWord.Word, embedding)) {
                embeddings.push_back(embedding);
            }
        }
        return embeddings;
    }

    TVector<TAnalyzedWord> AnalyzeWords(TStringBuf input) {
        TVector<TAnalyzedWord> analyzedWords;
        while (!input.empty()) {
            TString word = TString(input.NextTok(' '));
            TUtf16String wideWord = UTF8ToWide(word);
            TWLemmaArray foundLemmas;
            NLemmer::AnalyzeWord(wideWord.data(), wideWord.size(), foundLemmas, TLangMask(LANG_RUS, LANG_ENG));
            analyzedWords.push_back({std::move(wideWord), std::move(foundLemmas), 0.0f});
        }
        return analyzedWords;
    }

    const TStringBuf LEFT_QUERY = "сколько рук у человека";
    const TStringBuf RIGHT_QUERY = "сколько ног у человеков";

    void MakePreprocessorTestData(TVector<TAnalyzedWord>& leftInput, TVector<TAnalyzedWord>& rightInput) {
        leftInput = AnalyzeWords(LEFT_QUERY);
        rightInput = AnalyzeWords(RIGHT_QUERY);
    }

    void MakeCalculatorTestData(TVector<TEmbedding>& leftEmbeddings, TVector<TEmbedding>& rightEmbeddings, size_t& embeddingSize) {
        TVector<TAnalyzedWord> leftInput = AnalyzeWords(LEFT_QUERY);
        TVector<TAnalyzedWord> rightInput = AnalyzeWords(RIGHT_QUERY);
        THolder<TDictionary> dictionary(MakeDictionary());
        leftEmbeddings = GetEmbeddings(leftInput, dictionary.Get());
        rightEmbeddings = GetEmbeddings(rightInput, dictionary.Get());
        embeddingSize = dictionary->GetEmbeddingSize();
    }

}

Y_UNIT_TEST_SUITE(WordEmbeddingFeatures) {

    Y_UNIT_TEST(SubtractLemmasPreprocessorTest) {
        THolder<TPreprocessor> subtractLemmas = MakeHolder<TSubtractLemmasPreprocessor>("left", "right", "output");
        THashMap<TString, TVector<TAnalyzedWord>> data;

        TVector<TAnalyzedWord> leftInput, rightInput;
        MakePreprocessorTestData(leftInput, rightInput);

        data["left"] = leftInput;
        data["right"] = rightInput;
        subtractLemmas->Do(data);
        TVector<TUtf16String> outputLeftRight;
        for (const TAnalyzedWord& analyzedWord : data["output"]) {
            outputLeftRight.push_back(analyzedWord.Word);
        }
        UNIT_ASSERT_VALUES_EQUAL(u"рук", JoinStrings(outputLeftRight, u" "));

        data["left"] = rightInput;
        data["right"] = leftInput;
        data.erase("output");
        subtractLemmas->Do(data);
        TVector<TUtf16String> outputRightLeft;
        for (const TAnalyzedWord& analyzedWord : data["output"]) {
            outputRightLeft.push_back(analyzedWord.Word);
        }
        UNIT_ASSERT_VALUES_EQUAL(u"ног", JoinStrings(outputRightLeft, u" "));
    }

    Y_UNIT_TEST(SumSimCalculatorTest) {
        TVector<TEmbedding> leftEmbeddings, rightEmbeddings;
        size_t embeddingSize;
        MakeCalculatorTestData(leftEmbeddings, rightEmbeddings, embeddingSize);

        TFactFactorStorage features;
        TVector<EFactorId> factorIds = BuildFactorIds(0, 1);
        THolder<TFeaturesCalculator> calculator = MakeHolder<TSumSimCalculator>(std::move(factorIds), embeddingSize);
        calculator->Calculate(leftEmbeddings, rightEmbeddings, features);

        UNIT_ASSERT_DOUBLES_EQUAL(features[0], 0.961632013, std::numeric_limits<float>::epsilon());
    }

    Y_UNIT_TEST(MeanEuclideanDistCalculatorTest) {
        TVector<TEmbedding> leftEmbeddings, rightEmbeddings;
        size_t embeddingSize;
        MakeCalculatorTestData(leftEmbeddings, rightEmbeddings, embeddingSize);

        TFactFactorStorage features;
        TVector<EFactorId> factorIds = BuildFactorIds(0, 1);
        THolder<TFeaturesCalculator> calculator = MakeHolder<TMeanEuclideanDistCalculator>(std::move(factorIds), embeddingSize);
        calculator->Calculate(leftEmbeddings, rightEmbeddings, features);

        UNIT_ASSERT_DOUBLES_EQUAL(features[0], 0.19008255, std::numeric_limits<float>::epsilon());
    }

    Y_UNIT_TEST(VectorsToSumSimCalculator) {
        TVector<TEmbedding> leftEmbeddings, rightEmbeddings;
        size_t embeddingSize;
        MakeCalculatorTestData(leftEmbeddings, rightEmbeddings, embeddingSize);

        TVector<TEmbedding> vectors;
        vectors.push_back(TVector<float>(100, 0.1f));
        vectors.push_back(leftEmbeddings[0].Vec);
        TVector<EFactorId> factorIds = BuildFactorIds(0, 4);
        THolder<TFeaturesCalculator> calculator = MakeHolder<TVectorsToSumSimCalculator>(std::move(factorIds), vectors, embeddingSize);

        TFactFactorStorage features;
        calculator->Calculate(leftEmbeddings, rightEmbeddings, features);

        AssertValues(features, {0.105867833f, 0.0492197573f, 0.907035649f, 0.888858438f, 0.0f});
    }

    Y_UNIT_TEST(PairwiseSimStatsCalculator) {
        TVector<TEmbedding> leftEmbeddings, rightEmbeddings;
        size_t embeddingSize;
        MakeCalculatorTestData(leftEmbeddings, rightEmbeddings, embeddingSize);

        TVector<EFactorId> factorIds0 = BuildFactorIds(0, 6);
        THolder<TFeaturesCalculator> calculator1 = THolder<TFeaturesCalculator>(new TPairwiseSimStatsCalculator(
            std::move(factorIds0),
            {EPairwiseStat::GlobalMax, EPairwiseStat::GlobalMin,
             EPairwiseStat::LeftMinMax, EPairwiseStat::RightMinMax,
             EPairwiseStat::LeftMaxMin, EPairwiseStat::RightMaxMin},
            3, 3, 1, embeddingSize));
        TVector<EFactorId> factorIds1 = BuildFactorIds(6, 6);
        THolder<TFeaturesCalculator> calculator2 = THolder<TFeaturesCalculator>(new TPairwiseSimStatsCalculator(
            std::move(factorIds1),
            {EPairwiseStat::GlobalMax, EPairwiseStat::GlobalMin,
             EPairwiseStat::LeftMinMax, EPairwiseStat::RightMinMax,
             EPairwiseStat::LeftMaxMin, EPairwiseStat::RightMaxMin},
            3, 3, 2, embeddingSize));
        TFactFactorStorage features;
        calculator1->Calculate(leftEmbeddings, rightEmbeddings, features);
        calculator2->Calculate(leftEmbeddings, rightEmbeddings, features);

        const TVector<float> correctFeatures = {
            1.0f, 0.671332598f, 0.807885766f, 0.75121361f, 0.735846281f, 0.807885766f,
            0.910908103f, 0.871026874f, 0.907491088f, 0.907491088f, 0.884688377f, 0.884688377f,
            0.0f
        };

        AssertValues(features, correctFeatures);
    }
}
