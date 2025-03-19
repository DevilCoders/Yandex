#pragma once

#include "inthash.h"

#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/memory/segpool_alloc.h>

#include <library/cpp/numerator/numerate.h>

#include <bitset>

const size_t N_BASE_COUNT = 20;
const size_t N_MAX_WORDS = 5;
const size_t N_AVE_WORDS = 5;

const size_t N_INTEREST_TAGS = 15;
const size_t N_PARED_TAGS = 18;
const size_t N_FEATURES_COUNT = N_BASE_COUNT + N_MAX_WORDS + N_AVE_WORDS + N_INTEREST_TAGS + N_PARED_TAGS;
const size_t N_GRAMS = 3;
const size_t N_FREQ_WORDS = 325;

struct TTextFeatures
{
    float Features[N_FEATURES_COUNT];
    float& operator[](size_t i) {
        Y_ASSERT(i < N_FEATURES_COUNT);
        return Features[i];
    }
    float operator[](size_t i) const {
        Y_ASSERT(i < N_FEATURES_COUNT);
        return Features[i];
    }
};

class TFreqWords;
class IDocumentDataInserter;
class TTextFeaturesHandler : public INumeratorHandler
{
private:
    typedef TIntHash<ui32, 0x10000> TWordsHash;
    typedef TIntHash<ui32, 0x1000> TLinksHash;
    typedef TIntHash<ui32, 0x10000> TBigramsHash;
    TIntHash<ui32, 0x1000>::TPoolType HashPool;
    TWordsHash Words;
    TLinksHash Links;
    TBigramsHash Bigrams;
    // trigram (x,y,z) = code k -> pair(bigram code (x,y); number of entries)
    typedef TIntHash<std::pair<ui32, ui32>, 0x10000> TTrigramsType;
    TTrigramsType::TPoolType TrigramsPool;
    TTrigramsType Trigrams;
    TVector<ui32> LengCounts;

    ui32 WordsNum;
    ui32 CommentsLen;
    bool InScript;
    ui32 ScriptsLen;
    ui32 SumTagsEntry;
    ui32 SumWordsLen;

    TString DocUrl;
    TString Host;

    TVector<ui32> Tags;
    TVector<ui32> TagsStack;
    TVector<ui32> WordsInTags;
    TVector<ui32> LastWords;

    float TextVal;
    float LikeliHood;
    float PercentVisibleContent;
    float TrigramsProb, TrigramsCondProb;

    TFreqWords& FreqWords;
    ui32 FreqWordsNum;
    std::bitset<N_FREQ_WORDS> UsedFreqWords;

public:
    TTextFeaturesHandler();
    void SetDoc(const TString& docUrl);
    void OnTokenStart(const TWideToken& tok, const TNumerStat&) override;
    void OnZoneEntry(const TZoneEntry* zone);
    void OnMoveInput(const THtmlChunk& chunk, const TZoneEntry*, const TNumerStat&) override;
    void OnTextEnd(const IParsedDocProperties*, const TNumerStat& ) override;
    float GetTextVal() const;
    float GetLikeliHood() const;
    void Print(TTextFeatures& features);
    int GetNumWordsInText() const
    { return WordsNum; }
    // pared tag 0 is 'title', pared tags 14 is 'a'
    int GetNumWordsInTitle() const
    { return WordsInTags[0]; }
    float GetMeanWordLength() const
    {
        if (!WordsNum)
            return 0.f;
        float len = (float)SumWordsLen / (float)WordsNum;
        return len;
    }
    float GetPercentWordsInLinks() const
    { return WordsNum ? (float)WordsInTags[14] / (float)WordsNum : 0.f; }
    float GetPercentVisibleContent() const
    { return PercentVisibleContent; }
    float GetTrigramsProb() const
    { return TrigramsProb; }
    float GetTrigramsCondProb() const
    { return TrigramsCondProb; }
    float GetPercentFreqWords() const
    { return WordsNum ? (float)FreqWordsNum / (float)WordsNum : 0.f; }
    float GetPercentUsedFreqWords() const
    { return (float)UsedFreqWords.count() / (float)N_FREQ_WORDS; }
    void InsertFactors(IDocumentDataInserter& inserter) const;
};
