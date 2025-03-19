#pragma once

#include "nfa.h"
#include "dfa.h"
#include "debug.h"
#include "types.h"
#include "sized_array.h"
#include "submatch.h"

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/generic/stack.h>
#include <util/generic/ylimits.h>
#include <util/generic/bitmap.h>
#include <util/stream/debug.h>
#include <util/stream/output.h>
#include <util/system/defaults.h>
#include <utility>

#define DBGO GetDebugOutEXECUTE()
#define DBGOM GetDebugOutMATCH()

namespace NRemorph {

class OperationCounter {
public:
    OperationCounter(ui64 threshold = INFINITE_THRESHOLD)
        : Threshold(threshold)
        , NumberOfOperations(0)
        {
        }

    inline void Add(ui64 ops = 1) {
        NumberOfOperations += ops;
    }

    inline bool IsThresholdReached() const {
        return NumberOfOperations >= Threshold;
    }

    inline ui64 GetNumberOfOperations() const {
        return NumberOfOperations;
    }

private:
    ui64 Threshold;
    ui64 NumberOfOperations;

public:
    static const ui64 INFINITE_THRESHOLD = std::numeric_limits<ui64>::max();

};

struct TMatchInfo: public TSimpleRefCount<TMatchInfo> {
    TRuleId MatchedId;
    TVectorSubmatches Submatches;
    TNamedSubmatches NamedSubmatches;

    TMatchInfo()
        : MatchedId(0)
    {
    }
};

typedef TIntrusivePtr<TMatchInfo> TMatchInfoPtr;

namespace NPrivate {

class TTagIdToTagValues {
private:
    typedef TSizedArrayHolder<size_t> TArray;
private:
    const ui32* TagOffsets;
    TArray TagValues;

private:
    Y_FORCE_INLINE size_t* GetTagValues(TTagId tagId) {
        return &TagValues[TagOffsets[tagId]];
    }

    Y_FORCE_INLINE const size_t* GetTagValues(TTagId tagId) const {
        return &TagValues[TagOffsets[tagId]];
    }

    Y_FORCE_INLINE size_t GetTagValuesSize(TTagId tagId) const {
        return TagOffsets[tagId + 1] - TagOffsets[tagId];
    }

    inline void ExecuteStorePosition(const TCommands& cmd, size_t* tagValues, size_t tagValuesSize, size_t pos) {
        const ui8 index = cmd.GetValue(TCommands::StorePosition);
        Y_VERIFY(index < tagValuesSize, "Command=StorePosition, index=%d, tagValuesSize=%zd", (int)index, tagValuesSize);
        tagValues[index] = pos;
        NWRED(DBGO << "Execute: " << cmd.Tag << "[" << (int)index << "]<--" << pos << Endl);
    }

    inline void ExecuteStoreTag(const TCommands& cmd, const size_t* tagValues, size_t tagValuesSize) {
        const ui8 index = cmd.GetValue(TCommands::StoreTag);
        Y_VERIFY(index < tagValuesSize, "Command=StoreTag, index=%d, tagValuesSize=%zd", (int)index, tagValuesSize);
        FinalTagValues[cmd.Tag] = tagValues[index];
        NWRED(DBGO << "Execute: " << cmd.Tag << "[" << (int)index << "]" << "-->" << tagValues[index] << Endl);
    }

    inline void ExecuteShift(const TCommands& cmd, size_t* tagValues, size_t tagValuesSize) {
        const ui8 index = cmd.GetValue(TCommands::Shift);
        Y_VERIFY(index < tagValuesSize, "Command=Shift, index=%d, tagValuesSize=%zd", (int)index, tagValuesSize);
        for (size_t i = index; i < tagValuesSize; ++i) {
            tagValues[i - index] = tagValues[i];
        }
        NWRED(DBGO << "Execute: " << cmd.Tag << "<<" << (int)index << Endl);
    }
public:
    TArray FinalTagValues;
public:
    TTagIdToTagValues(const TDFA& dfa, size_t curPos)
        : TagOffsets(dfa.GetTagOffsets())
        , TagValues(dfa.GetTagOffsets()[dfa.GetMaxTags()])
        , FinalTagValues(dfa.GetMaxTags())
    {
        Execute(dfa.GetInitCmdBeg(), dfa.GetInitCmdEnd(), curPos);
    }

    void Execute(const TCommands* begin, const TCommands* end, size_t pos) {
        for (const TCommands* it = begin; it != end; ++it) {
            size_t tagValuesSize = GetTagValuesSize(it->Tag);
            if (it->HasCommand(TCommands::Shift)) {
                ExecuteShift(*it, GetTagValues(it->Tag), tagValuesSize);
            }
            if (it->HasCommand(TCommands::StorePosition)) {
                ExecuteStorePosition(*it, GetTagValues(it->Tag), tagValuesSize, pos);
            }
            if (it->HasCommand(TCommands::StoreTag)) {
                ExecuteStoreTag(*it, GetTagValues(it->Tag), tagValuesSize);
            }
        }
    }
};

template <class TSymbolIterator>
struct TStep {
    TSymbolIterator Iter;
    const TDFATransition* Tr;
    size_t Pos;

    size_t PrevStep;
    size_t Refs;

    TStep(const TSymbolIterator& iter, const TDFATransition* tr = nullptr, size_t prev = 0)
        : Iter(iter)
        , Tr(tr)
        , Pos(iter.GetCurPos())
        , PrevStep(prev)
        , Refs(0)
    {
    }

    void Set(const TSymbolIterator& iter, const TDFATransition* tr, size_t prev) {
        Iter = iter;
        Tr = tr;
        Pos = iter.GetCurPos();
        PrevStep = prev;
        Refs = 0;
    }
};

template <class TSymbolIterator>
class TStepCollection {
private:
    enum {
        PageSize = 1u << 6
    };
    TVector<TStep<TSymbolIterator>*> Pages;
    size_t CurPageCount;
    TDynBitMap FreeSteps;
private:

    TStep<TSymbolIterator>* Alloc() {
        if (CurPageCount >= PageSize) {
            Pages.push_back((TStep<TSymbolIterator>*)::malloc(sizeof(TStep<TSymbolIterator>) * PageSize));
            CurPageCount = 0;
        }
        return (Pages.back() + CurPageCount++);
    }

    static void Destroy(TStep<TSymbolIterator>* page, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            (page + i)->~TStep<TSymbolIterator>();
        }
        ::free(page);
    }

public:
    TStepCollection()
        : CurPageCount(0)
    {
        Pages.push_back((TStep<TSymbolIterator>*)::malloc(sizeof(TStep<TSymbolIterator>) * PageSize));
    }

    ~TStepCollection() {
        if (!Pages.empty()) {
            Destroy(Pages.back(), CurPageCount);
            Pages.pop_back();
            for (size_t p = 0; p < Pages.size(); ++p) {
                Destroy(Pages[p], PageSize);
            }
        }
    }

    inline size_t Size() const {
        return Pages.empty() ? 0 : (Pages.size() - 1) * PageSize + CurPageCount;
    }

    inline NPrivate::TStep<TSymbolIterator>& At(size_t n) {
        Y_ASSERT(n / PageSize < Pages.size());
        Y_ASSERT(n / PageSize < Pages.size() + 1 || n % PageSize < CurPageCount);
        return Pages[n / PageSize][n % PageSize];
    }

    inline const NPrivate::TStep<TSymbolIterator>& At(size_t n) const {
        Y_ASSERT(n / PageSize < Pages.size());
        Y_ASSERT(n / PageSize < Pages.size() + 1 || n % PageSize < CurPageCount);
        return Pages[n / PageSize][n % PageSize];
    }

    Y_FORCE_INLINE NPrivate::TStep<TSymbolIterator>& operator[](size_t n) {
        return At(n);
    }

    Y_FORCE_INLINE const NPrivate::TStep<TSymbolIterator>& operator[](size_t n) const {
        return At(n);
    }

    inline bool HasFreeSteps(size_t limit) const {
        return !FreeSteps.Empty() || Size() < limit;
    }

    size_t GetNewStep(const TSymbolIterator& it, const TDFATransition* tr, size_t prev) {
        size_t newStep;
        if (FreeSteps.Empty()) {
            newStep = Size();
            new (Alloc()) NPrivate::TStep<TSymbolIterator>(it, tr, prev);
        } else {
            newStep = FreeSteps.FirstNonZeroBit();
            FreeSteps.Reset(newStep);
            Y_ASSERT(At(newStep).Refs == 0);
            At(newStep).Set(it, tr, prev);
        }
        ++At(prev).Refs;
        return newStep;
    }

    void CheckAndReleaseSteps(size_t checkSt) {
        while (checkSt != 0 && 0 == At(checkSt).Refs) {
            FreeSteps.Set(checkSt);
            checkSt = At(checkSt).PrevStep;
            --At(checkSt).Refs;
        }
    }
};

template <class TSymbolIterator>
inline size_t GetNextCount(TLiteral lit, const TSymbolIterator& iter) {
    return lit.IsZeroShift() ? 1 : iter.GetNextCount();
}

template <class TSymbolIterator>
inline TSymbolIterator GetNextIter(TLiteral lit, const TSymbolIterator& iter, size_t n) {
    Y_ASSERT(!lit.IsZeroShift() || 0 == n);
    return lit.IsZeroShift() ? iter : iter.GetNext(n);
}

} // NPrivate


template <class TSymbolIterator>
class TMatchState {
private:
    const TDFA& Dfa;
    TVector<const NPrivate::TStep<TSymbolIterator>*> Steps;
    const TSymbolIterator& Iter;

public:
    TMatchState(const TDFA& dfa, const NPrivate::TStepCollection<TSymbolIterator>& steps, size_t top)
        : Dfa(dfa)
        , Iter(steps[top].Iter)
    {
        for (size_t step = top; step != 0; step = steps[step].PrevStep) {
            Y_ASSERT(step < steps.Size());
            Steps.push_back(&steps[step]);
        }
        Y_ASSERT(!Steps.empty());
        Steps.push_back(&steps[0]);
        // We traversed steps from back to top. Now, revert them in direct order
        ::Reverse(Steps.begin(), Steps.end());
    }

    const TSymbolIterator& GetIter() const {
        return Iter;
    }

    template <class TOperator>
    bool PostMatch(TOperator op) const {
        // Skip start step because it have no transition
        for (size_t i = 1; i < Steps.size(); ++i) {
            Y_ASSERT(Steps[i]->Tr != nullptr);
            if (!Steps[i]->Tr->Lit.IsAnchor() && op(Steps[i]->Tr->Lit)) {
                return false;
            }
        }
        return true;
    }

    // In most cases only single MatchInfo will be returned.
    // Multiple MatchInfos appear when DFA has several rules, which differ only by submatches.
    // In that case the final state corresponds to several rules, each of which has a different collection of submatches
    TVector<TMatchInfoPtr> CreateMatchInfos() const {
        Y_ASSERT(Steps.back()->Tr != nullptr);
        TStateId finalState = Steps.back()->Tr->To;

        Y_ASSERT(Dfa.GetState(finalState).FinalCount > 0);
        TVector<TMatchInfoPtr> res;
        res.reserve(Dfa.GetState(finalState).FinalCount);

        NPrivate::TTagIdToTagValues matchTagValues(Dfa, Steps.front()->Pos);

        // Skip start step because it have no transition
        for (size_t i = 1; i < Steps.size(); ++i) {
            const TDFATransition* tr = Steps[i]->Tr;
            Y_ASSERT(tr != nullptr);
            matchTagValues.Execute(Dfa.GetCmdBeg(*tr), Dfa.GetCmdEnd(*tr), Steps[i]->Pos);
        }

        TSizedArrayHolder<size_t> finalTagValuesBackup(matchTagValues.FinalTagValues);

        const TFinalCommands* iBegin = Dfa.GetFinalsBeg(finalState);
        const TFinalCommands* iEnd = Dfa.GetFinalsEnd(finalState);
        for (const TFinalCommands* iFinal = iBegin; iFinal != iEnd; ++iFinal) {

            res.push_back(new TMatchInfo());
            TMatchInfo& matchInfo = *res.back();
            matchInfo.MatchedId = iFinal->Rule;
            // Restore final values for all iterations except first one
            if (iFinal != iBegin) {
                matchTagValues.FinalTagValues = finalTagValuesBackup;
            }
            // Execute commands for the specific rule
            matchTagValues.Execute(Dfa.GetCmdBeg(*iFinal), Dfa.GetCmdEnd(*iFinal), Steps.back()->Pos);
            TTagId submatchIdFirst = Dfa.SubmatchIdOffsets[matchInfo.MatchedId];
            TTagId submatchIdLast = Dfa.SubmatchIdOffsets[matchInfo.MatchedId + 1];
            matchInfo.Submatches.resize(submatchIdLast - submatchIdFirst);
            for (TTagId submatchId = submatchIdFirst; submatchId < submatchIdLast; ++submatchId) {
                TTagId tagId = submatchId * 2;
                TSubmatch& s = matchInfo.Submatches[submatchId - submatchIdFirst];
                s.first = matchTagValues.FinalTagValues[tagId];
                s.second = matchTagValues.FinalTagValues[tagId + 1];
                if (s.first == s.second) {
                    s.first = -1;
                    s.second = -1;
                } else {
                    TSubmatchIdToName::const_iterator it = Dfa.SubmatchIdToName.find(submatchId);
                    if (it != Dfa.SubmatchIdToName.end()) {
                        matchInfo.NamedSubmatches.insert(std::make_pair(it->second, s));
                    }
                }
            }
        }
        return res;
    }
};

// Returns true if all possible variants are processed,
// and false if the limit for number of states has been reached
template <class TLiteralTable, class TSymbolIterator, class TOnMatch>
static inline bool SearchFrom(const TLiteralTable& lt, const TDFA& dfa, const TSymbolIterator& from,
    TOnMatch& onMatch, size_t limit = Max<size_t>(), OperationCounter* opcnt = nullptr) {
    Y_ASSERT(limit > 0);

    bool full = true;

    NPrivate::TStepCollection<TSymbolIterator> steps;
    TDynBitMap topSteps;
    TDynBitMap newTopSteps;

    topSteps.Set(steps.GetNewStep(from, nullptr, 0));

    while (!topSteps.Empty() && onMatch) {
        bool breakPhase = false;
        for (size_t prevStep = topSteps.FirstNonZeroBit(); prevStep < topSteps.Size() && onMatch && !breakPhase; prevStep = topSteps.NextNonZeroBit(prevStep)) {
            Y_ASSERT(prevStep < steps.Size());
            Y_ASSERT(prevStep == 0 || nullptr != steps[prevStep].Tr);
            const TStateId state = Y_UNLIKELY(0 == prevStep) ? 0 : steps[prevStep].Tr->To;
            const TDFATransition* iTrBegin = dfa.GetTransBeg(state);
            const TDFATransition* iTrEnd = dfa.GetTransEnd(state);
            for (const TDFATransition* iTr = iTrBegin; iTr != iTrEnd && onMatch && !breakPhase; ++iTr) {
                if (!Compare(lt, iTr->Lit, steps[prevStep].Iter)) {
                    continue;
                }
                const size_t cnt = NPrivate::GetNextCount(iTr->Lit, steps[prevStep].Iter);
                for (size_t nit = 0; nit < cnt && onMatch; ++nit) {
                    if (opcnt)
                        opcnt->Add();
                    if (Y_UNLIKELY(!steps.HasFreeSteps(limit) || (opcnt && opcnt->IsThresholdReached()))) {
                        full = false;
                        breakPhase = true;
                        break;
                    }
                    size_t newStep = steps.GetNewStep(NPrivate::GetNextIter(iTr->Lit, steps[prevStep].Iter, nit), iTr, prevStep);
                    newTopSteps.Set(newStep);

                    if (dfa.GetState(iTr->To).IsFinal()) {
                        onMatch(TMatchState<TSymbolIterator>(dfa, steps, newStep));
                    }
                }
            }
            steps.CheckAndReleaseSteps(prevStep);
        }
        DoSwap(topSteps, newTopSteps);
        newTopSteps.Clear();
    }

    return full;
}

} // NRemorph

Y_DECLARE_OUT_SPEC(inline, NRemorph::TMatchInfo, s, r) {
    s << r.MatchedId << "," << r.Submatches << "," << r.NamedSubmatches;
}

#undef DBGO
#undef DBGOM
