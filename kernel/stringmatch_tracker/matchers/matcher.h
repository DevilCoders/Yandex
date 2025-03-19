#pragma once

#include "charmap.h"
#include "ref_charmap.h"

#include <kernel/factor_storage/factor_storage.h>

#include <library/cpp/charset/codepage.h>
#include <util/generic/hash.h>

namespace NSequences {

class TLCSMatcher {
private:
    // 26 - size of English alphabet (must be synchronized with NPrivate::charmap)
    static const size_t AlphabetSize = 26;

private:
    struct TState {
        size_t Length = 0;
        size_t Link = 0;
        ui32 Next[AlphabetSize];

        void Clear() {
            Length = 0;
            Link = 0;
            memset(Next, 0, sizeof(Next));
        }
    };

    TVector<TState> States;

    // for building SA
    size_t Size = 1;
    size_t LastStateNum = 0;

    // for finding lcs
    size_t CurState = 0;
    size_t CurLength = 0;
    size_t BestLength = 0;

    // common
    const size_t Threshold = 3; // min value, when match is non-zero

    void ExtendSA(const ui8 ch);

public:
    // default threshold = 1, cause 0 is useless
    TLCSMatcher(const TString& request, size_t threshold = 1);

public:
    void ExtendMatch(char c);

    void ResetMatch() {
        BestLength = 0;
        CurState = 0;
        CurLength = 0;
    }

    size_t GetCurMatch() const {
        return BestLength < Threshold ? 0 : BestLength;
    }
};


class TTrigramMatcher {
private:
    enum ESize {
        QUERY_SEQ_SIZE = 1 << 18
    };

private:
    ui8 QuerySequences[QUERY_SEQ_SIZE / 8];
    ui8 QueryHash[256];

    ui32 CurrentMatch;

private:
    void DoFill(const TStringBuf& str);
    void FillFromRequest(const TString& request, bool fillByWords = true);

public:
    TTrigramMatcher()
        : CurrentMatch(0)
    {
        memset(QueryHash, 0, sizeof(QueryHash));
        memset(QuerySequences, 0, sizeof(QuerySequences));
    }

    TTrigramMatcher(const TString& request, bool fillByWords = true)
        : CurrentMatch(0)
    {
        memset(QueryHash, 0, sizeof(QueryHash));
        memset(QuerySequences, 0, sizeof(QuerySequences));
        FillFromRequest(ToLower(request, csYandex), fillByWords);
    }

    void ResetMatch() {
        CurrentMatch = 0;
    }

    bool FindMatch() {
        return GetBit(CurrentMatch);
    }

    ui32 GetCurrentMatch() const {
        return CurrentMatch;
    }

    void ExtendMatch(char c) {
        ui8 ch = NPrivate::charmap[ui8(c)];
        if (!ch)  // see OXYGEN-178
            return;

        // assertion about lowercase symbols was here
        CurrentMatch >>= 6;
        CurrentMatch += ui32(ch) << 12;
    }

    void SetBit(size_t s) {
        Y_ASSERT(s < QUERY_SEQ_SIZE);
        QuerySequences[s >> 3] |= (1 << (s & 7));
        QueryHash[s & 0xff] = 1;
    }

    bool GetBit(size_t s) const {
        Y_ASSERT(s < QUERY_SEQ_SIZE);
        if (QueryHash[s & 0xff] == 0)
            return 0;
        return (QuerySequences[s >> 3] & (1 << (s & 7))) != 0;
    }

    void Clear() {
        CurrentMatch = 0;
        memset(QueryHash, 0, sizeof(QueryHash));
        memset(QuerySequences, 0, sizeof(QuerySequences));
    }
};


class TExtendedTrigramMatcher: public TTrigramMatcher {
private:
    TTrigramMatcher TextMatcher;
    ui32 QueryUniqTrigrams = 0;
    ui32 TextUniqTrigrams = 0;
    ui32 MatchedTrigrams = 0;

private:
    double CalcFactor(size_t numMatched, size_t length) const {
        double factor = 0;

        if (length) {
            const ui8 value = (ui8)(1.0 + double(numMatched) * 254 / double(length));
            factor = Ui82Float(value);
        }
        return factor;
    }

public:
    void InitFromRequest(const char* const beg, const char* const end);

    void AddText(const char* const beg, const char* const end, ui32* const trigramMatch);

    void GetFactors(float* matchedToQueryTrigrams, float* matchedToTextTrigrams) const {
        *matchedToQueryTrigrams = CalcFactor(MatchedTrigrams, QueryUniqTrigrams);
        *matchedToTextTrigrams = CalcFactor(MatchedTrigrams, TextUniqTrigrams);
    }

    void ClearText() {
        TextUniqTrigrams = 0;
        MatchedTrigrams = 0;
        TextMatcher.Clear();
    }

};

}

namespace NRefSequences {

template<int MskSz>
bool AddTrigramm(ui64* fuzzySet, ui32 trigram) {
    const size_t numBins = MskSz * 64;
    const ui32 binId = trigram % numBins;
    const ui32 idx = binId / 64;
    const ui32 bitId = binId % 64;
    const ui64 one = 1;
    const ui64 mask = one << bitId;
    if (fuzzySet[idx] & mask) {
        return false;
    }
    fuzzySet[idx] |= mask;
    return true;
}


class TBinaryMaskTrigramMatcher {
public:
    TBinaryMaskTrigramMatcher()
    {
        memset(FuzzyTrigrammSet, 0, sizeof(FuzzyTrigrammSet));
        memset(QueryTrigrammSet, 0, sizeof(QueryTrigrammSet));
    }

    void CalcFactors(float& queryInText, float& textInQuery, size_t& numMatched) {
        numMatched = NumUniqMatchedTgs;
        if (NumUniqQueryTgs) {
            queryInText = numMatched / static_cast<float>(NumUniqQueryTgs);
        }
        if (NumUniqTextTgs) {
            textInQuery = ClampVal(numMatched / static_cast<float>(NumUniqTextTgs), 0.0f, 1.0f);
        }
    }

    void AddTextTrigram(ui32 tg) {
        if (AddTrigramm<MskSz>(FuzzyTrigrammSet, tg))
            ++NumUniqTextTgs;
        if (CheckQueryTrigramm(tg)) {
            ResetQueryTrigramm(tg);
            ++NumUniqMatchedTgs;
        }
    }

    void AddQueryTrigram(ui32 tg) {
        if (CheckQueryTrigramm(tg)) {
            return;
        }
        SetQueryTrigramm(tg);
        ++NumUniqQueryTgs;
    }

    bool FinishQuery() {
        return NumUniqQueryTgs;
    }

    bool CheckQueryTrigramm(ui32 binId) {
        ui32 idx = binId / 64;
        ui32 bitId = binId % 64;
        ui64 one = 1;
        ui64 mask = one << bitId;
        return QueryTrigrammSet[idx] & mask;
    }

    void SetQueryTrigramm(ui32 binId) {
        ui32 idx = binId / 64;
        ui32 bitId = binId % 64;
        ui64 one = 1;
        ui64 mask = one << bitId;
        QueryTrigrammSet[idx] |= mask;
    }

    void ResetQueryTrigramm(ui32 binId) {
        ui32 idx = binId / 64;
        ui32 bitId = binId % 64;
        ui64 one = 1;
        ui64 mask = one << bitId;
        QueryTrigrammSet[idx] &= ~mask;
    }
private:
    size_t NumUniqTextTgs = 0;
    size_t NumUniqMatchedTgs = 0;
    size_t NumUniqQueryTgs = 0;
    static const size_t MskSz = 1013;
    ui64 FuzzyTrigrammSet[MskSz];
    ui64 QueryTrigrammSet[NRefSequences::TgMaskSz];
};


class TBinarySearchTrigramMatcher {
public:
    explicit TBinarySearchTrigramMatcher()
        : NumUniqTextTgs(0)
        , NumUniqMatchedTgs(0)
    {
        memset(FuzzyTrigrammSet, 0, sizeof(FuzzyTrigrammSet));
    }

    void CalcFactors(float& queryInText, float& textInQuery, size_t& numMatched) {
        numMatched = NumUniqMatchedTgs;
        if (QueryGramms.size()) {
            queryInText = numMatched / (float)QueryGramms.size();
        }
        if (NumUniqTextTgs) {
            textInQuery = numMatched / (float)NumUniqTextTgs;
        }
    }

    void AddTextTrigram(ui32 tg) {
        if (AddTrigramm<MskSz>(FuzzyTrigrammSet, tg)) {
            ++NumUniqTextTgs;
        }
        auto it = std::lower_bound(QueryGramms.begin(), QueryGramms.end(), tg);
        size_t gno = it - QueryGramms.begin();
        if (it == QueryGramms.end() || *it != tg || SeenGramms[gno]) {
            return;
        }
        SeenGramms[gno] = 1;
        ++NumUniqMatchedTgs;
    }

    void AddQueryTrigram(ui32 tg) {
        QueryGramms.push_back(tg);
    }

    bool FinishQuery() {
        std::sort(QueryGramms.begin(), QueryGramms.end());
        auto it = std::unique(QueryGramms.begin(), QueryGramms.end());
        QueryGramms.resize(it - QueryGramms.begin());
        SeenGramms.clear();
        SeenGramms.resize(QueryGramms.size(), false);
        return QueryGramms.size();
    }
private:
    TVector<ui32> QueryGramms; //can make fixed size 256
    TVector<bool> SeenGramms;  //can make fixed size 256
    size_t NumUniqTextTgs;
    size_t NumUniqMatchedTgs;
    static const size_t MskSz = 1013;
    ui64 FuzzyTrigrammSet[MskSz];
};


} // namespace NRefSequences
