#pragma once

#include <queue>
#include <cmath>

#include <library/cpp/langmask/langmask.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <utility>

template <typename TVal>
struct TTTrieFinderRet;
using TWtringTrieFinderRet = TTTrieFinderRet<const wchar16*>;

template <typename TChr>
struct TTrieNode;

namespace NLemmer {
    namespace NDetail {
        struct TNGramData;
        class TAlphaMap;
    }
}

class TUntransliter {
public:
    enum {
        WeightDelimiter = wchar16(-1),
        WeightBase = wchar16(1)
    };
    static wchar16 EncodeWeight(ui32 wt) {
        Y_ASSERT(wt < 60000);
        return static_cast<wchar16>(wt + WeightBase);
    }
    static ui32 DecodeWeight(wchar16 c) {
        return static_cast<ui32>(c - WeightBase);
    }

public:
    class WordPart {
        TUtf16String Word;
        size_t Ind;
        ui32 Weight;

    public:
        WordPart()
            : Word()
            , Ind(0)
            , Weight(0){};
        const TUtf16String& GetWord() const {
            return Word;
        }
        ui32 Quality() const {
            return Ind ? static_cast<ui32>(Weight / (log(static_cast<double>(Ind)) + 1.0) + 0.5) : 0;
        }
        bool Empty() const {
            return Ind == 0;
        }

    private:
        size_t AddPart(const wchar16*& trRes, size_t num);
        void AddWeight(ui32 weight);
        size_t GetInd() const;
        void MakeCapt(TCharCategory capt);
        friend bool operator<(const WordPart& w1, const WordPart& w2);
        friend class TUntransliter;
    };

private:
    const TTrieNode<wchar16>* Trie;
    const wchar16** TrieData;

    std::priority_queue<WordPart> Que;
    TUtf16String Word;
    TCharCategory Capt;

    class TNgrChecker;
    THolder<TNgrChecker> NGr;

public:
    static const size_t MaxUntransliterableLength = 32;
    static const size_t WorstQuality = 10000; //с потолка
    static const double DefaultQualityRange;  // = 2.7128...
public:
    TUntransliter(const TTrieNode<wchar16>* trie, const wchar16** trieData, const NLemmer::NDetail::TNGramData& nGram, const NLemmer::NDetail::TAlphaMap& alphaMap);
    virtual ~TUntransliter();

    void Init(const TWtringBuf& word);
    WordPart GetNextAnswer(ui32 worstQuality = 0);

private:
    void NextLetter(const WordPart& wp, TVector<TWtringTrieFinderRet>& pars, ui32 worstQuality = 0);
    virtual bool Unacceptable(const TUtf16String& str, size_t diff) const;
};

bool operator<(const TUntransliter::WordPart& w1, const TUntransliter::WordPart& w2);
