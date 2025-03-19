#include <util/generic/algorithm.h>

#include "lexical_decomposition_algo.h"

using namespace NLexicalDecomposition;

TDecompositionResultDescr::TDecompositionResultDescr(TDescrResult descr, ui32 frequency, ui32 badness) {
    if (badness <= MAX_BADNESS)
        Measure = (ui64)((badness ^ Max<ui8>()) << 8 | ((ui8)descr)) << 32 | frequency;
    else
        Measure = 0;
}

bool TDecompositionResultDescr::Relax(const TDecompositionResultDescr& rhs) {
    if (IsWorseThan(rhs)) {
        *this = rhs;
        return true;
    }
    return false;
}

TLexicalDecomposition::TLexicalDecomposition(ui32 options, ui32 tokenLength, const TEndposPtr& ends, const TAdditionalInfoPtr& additionalInfo,
                                             const TDecompositionResultDescr& descr)
    : Options(options)
    , TokenLength(tokenLength)
    , Ends(ends)
    , AdditionalInfo(additionalInfo)
    , TokenAmount(Ends->size())
    , Descr(descr)
    , Improved(false)
{
    if (!Ends.Get() || !AdditionalInfo.Get())
        ythrow yexception() << "TLexicalDecomposition constructor: some arguments are empty";
    if (Ends->size() != AdditionalInfo->size())
        ythrow yexception() << "TLexicalDecomposition constructor: argument sizes do not match";
}

static TDecompositionResultDescr::TDescrResult TripleFlag2Descr(ui32 flag) {
    typedef TDecompositionResultDescr T;
    static const ui32 CNT = 7;
    static const T::TDescrResult flag2descr[CNT] = {T::DR_VVV, T::DR_SVV, T::DR_VSV, T::DR_EMPTY, T::DR_VVS, T::DR_EMPTY, T::DR_VSS};
    if (flag >= CNT) {
        return T::DR_EMPTY;
    }
    return flag2descr[flag];
}

bool TLexicalDecomposition::DoDecompositionManual() {
    if (TokenLength <= 1 || TokenAmount == 0) {
        return false;
    }

    typedef TVector<ui32> TInfoArr;
    const size_t N = TokenLength;
    ui32 initValue = INFTY;
    TInfoArr direct(N + 1, initValue);  /// direct[i] means occurence 0..i-1, equals to index of corresponding word in the info array
    TInfoArr inverse(N + 1, initValue); /// the same information about the i..N-1 occurence
    for (size_t i = 0; i < TokenAmount; ++i) {
        if (GetEnd(i) < GetLength(i)) {
            ythrow yexception() << "invalid searcher result data [" << i << "]: (" << GetEnd(i) << ", " << GetLength(i) << ")" << Endl;
        }
        if (GetEnd(i) == GetLength(i)) {
            direct[GetLength(i)] = i;
        }
        if (GetEnd(i) == N) {
            inverse[N - GetLength(i)] = i;
        }
    }

    TDecompositionResultDescr descr(Descr);
    size_t bestI = 0;

    /// 2-words stage
    for (size_t i = 0; i < N; ++i) {
        if (direct[i] != INFTY && inverse[i] != INFTY) {
            TDecompositionResultDescr current(
                TDecompositionResultDescr::DR_WW,
                GetFreq(direct[i]) + GetFreq(inverse[i]),
                GetBadness(direct[i]) + GetBadness(inverse[i]));
            if (descr.Relax(current))
                bestI = i;
        }
    }

    /// 3-words stage
    for (size_t i = 0; i < TokenAmount; ++i) {
        ui32 w1 = direct[GetEnd(i) - GetLength(i)];
        ui32 w3 = inverse[GetEnd(i)];
        if (w1 != INFTY && w3 != INFTY) {
            TDecompositionResultDescr current(
                TripleFlag2Descr(IsStopWord(w1) + IsStopWord(i) * 2 + IsStopWord(w3) * 4),
                GetFreq(w1) + GetFreq(i) + GetFreq(w3),
                GetBadness(w1) + GetBadness(i) + GetBadness(w3));
            if (descr.Relax(current))
                bestI = i;
        }
    }

    /// restoring the answer if it good enough
    if (Descr.Relax(descr)) {
        Result.clear();
        size_t i = bestI;
        if (descr.GetDescr() == TDecompositionResultDescr::DR_WW) {
            PushWord(direct[i]);
            PushWord(inverse[i]);
        } else {
            PushWord(direct[GetEnd(i) - GetLength(i)]);
            PushWord(i);
            PushWord(inverse[GetEnd(i)]);
        }
        return (Improved = true);
    }
    return false;
}

template <typename T1, typename T2>
static inline void Relax(T1& lhs1, T2& lhs2, const T1& rhs1, const T2& rhs2) {
    if (lhs1 < rhs1) {
        lhs1 = rhs1;
        lhs2 = rhs2;
    }
}

bool TLexicalDecomposition::DoDecomposition() {
    if (TokenLength <= 1 || TokenAmount == 0) {
        return false;
    }
    //todo: use the frequency information in DP
    /// (max letters covered, -(min words used for it)), minus is for internal std::pair comparison use
    typedef TVector<std::pair<ui32, int>> TInfoArr;
    /// needs to restore the answer
    typedef TVector<ui32> TRestoreArr;

    /// [i] == best cover pair for prefix token[0..i-1]
    TInfoArr bestCover(TokenLength + 1, std::make_pair(0, 0));
    TRestoreArr lastPickedWord(TokenLength + 1, INFTY);

    size_t infoI = 0;
    size_t restoreI = 0;
    /// uses the fact that Ends are sorted in increasing order
    for (size_t i = 0; i < TokenAmount; ++i) {
        while (infoI != GetEnd(i)) {
            Relax(bestCover[infoI + 1], lastPickedWord[restoreI + 1], bestCover[infoI], lastPickedWord[restoreI]);
            ++infoI;
            ++restoreI;
        }
        ui32 wordLength = GetLength(i);
        size_t wherePrev = GetEnd(i) - wordLength;
        const std::pair<ui32, int> pickCurrentWord(bestCover[wherePrev].first + wordLength, bestCover[wherePrev].second - 1);
        Relax(bestCover[infoI], lastPickedWord[restoreI], pickCurrentWord, (ui32)i);
    }
    while (infoI != TokenLength) {
        Relax(bestCover[infoI + 1], lastPickedWord[restoreI + 1], bestCover[infoI], lastPickedWord[restoreI]);
        ++infoI;
        ++restoreI;
    }

    /// statistics to create TDecompositionResult
    ui32 badness = 0;
    ui32 frequency = 0;
    {
        ui32 prefix = TokenLength;
        while (lastPickedWord[prefix] != INFTY) {
            size_t wordId = lastPickedWord[prefix];
            if (GetEnd(wordId) < prefix)
                ++badness;
            badness += GetBadness(wordId);
            frequency += GetFreq(wordId);
            prefix = GetEnd(wordId) - GetLength(wordId);
        }
        if (prefix > 0)
            ++badness;
    }

    if (Descr.Relax(TDecompositionResultDescr(TDecompositionResultDescr::DR_GENERAL, frequency, badness))) {
        Result.clear();
        /// now restore the answer
        ui32 prefix = TokenLength;
        while (lastPickedWord[prefix] != INFTY) {
            size_t wordId = lastPickedWord[prefix];
            if (GetEnd(wordId) < prefix)
                PushWord(INFTY, prefix - GetEnd(wordId)); /// negative number
            PushWord(wordId);
            prefix = GetEnd(wordId) - GetLength(wordId);
        }
        if (prefix > 0)
            PushWord(INFTY, prefix);
        std::reverse(Result.begin(), Result.end());
        return (Improved = true);
    }
    return false;
}

bool TLexicalDecomposition::ResultIsUgly() const {
    if ((Options & DO_4WORDSMAX) && Result.size() > 4) {
        return true;
    }
    if (Descr.GetBadness() > 1) {
        return true;
    }
    bool fullCover = true;
    ui32 maximalLength = 0;
    for (size_t i = 0; i < Result.size(); ++i) {
        fullCover &= VocabularyElement(Result[i]);
        if (!(Options & DO_ORFO) || VocabularyElement(Result[i])) {
            maximalLength = Max(maximalLength, GetLength(Result[i]));
        }
    }
    if ((Options & DO_COMPLETE) && !fullCover) {
        return true;
    }
    /// heuristic
    if (maximalLength <= 3 || maximalLength <= Result.size()) {
        return true;
    }

    return false;
}
