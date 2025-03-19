#include "ngrams_processor.h"
#include <util/string/split.h>

TNgramsProcessor::TNgramsProcessor(const char* clustersFile, const char* modelFile)
    : Model()
    , AhoWords(nullptr)
    , FeatureCount(0)
{
    FeatureCount = LoadClusters(clustersFile);
    LoadModel(modelFile);
}

// This constructor gives an opportunity to create processor
// without model (for example, to call CalculateFeatures)
TNgramsProcessor::TNgramsProcessor(const char* clustersFile)
    : Model()
    , AhoWords(nullptr)
    , FeatureCount(0)
{
    FeatureCount = LoadClusters(clustersFile);
}

TNgramsProcessor::~TNgramsProcessor() {
    delete AhoWords;
}

float TNgramsProcessor::Match(const char* url) const {
    TVector<float> featuresVector;
    CalculateFeatures(url, &featuresVector);

    double answer = 0.0;
    const float* pFactors = &featuresVector.front();
    Model.DoCalcRelevs(&pFactors, &answer, 1);
    return answer;
}

void TNgramsProcessor::LoadModel(const char* modelFile) {
    TFileInput input(modelFile);
    ::Load(&input, Model);
}

size_t TNgramsProcessor::LoadClusters(const char* clustersFile) {
    TFileInput input(clustersFile);

    TDefaultAhoCorasickBuilder AhoWordsBuilder;
    size_t lineIndex = 0;
    for (TString line; input.ReadLine(line); ++lineIndex) {
        TVector<TString> words;
        StringSplitter(line).SplitBySet(" \t").SkipEmpty().Collect(&words);
        for (size_t wordIndex = 0; wordIndex < words.size(); ++wordIndex) {
            AhoWordsBuilder.AddString(words[wordIndex], lineIndex);
        }
    }
    TBufferStream buffer;
    AhoWordsBuilder.SaveToStream(&buffer);
    WordsBlob = TBlob::FromStream(buffer);
    AhoWords = new TDefaultMappedAhoCorasick(WordsBlob);
    return lineIndex;
}

void TNgramsProcessor::CalculateFeatures(const char* url, TVector<float>* featuresVector) const {
    *featuresVector = TVector<float>(FeatureCount, 0.0);
    TDefaultMappedAhoCorasick::TSearchResult ahoSearchResultBuffer;

    TString urlStroka = url;
    urlStroka.to_lower();
    AhoWords->AhoSearch(urlStroka, &ahoSearchResultBuffer);
    for (TDefaultMappedAhoCorasick::TSearchResult::const_iterator it = ahoSearchResultBuffer.begin();
         it != ahoSearchResultBuffer.end();
         ++it)
    {
        (*featuresVector)[it->second] = 1.0;
    }
}
