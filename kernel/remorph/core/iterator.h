#pragma once

#include "input_tree.h"
#include "literal.h"
#include "util.h"

#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/bitmap.h>

namespace NRemorph {

struct TTrackStep;
typedef TIntrusivePtr<TTrackStep> TTrackStepPtr;

struct TTrackStep: public TSimpleRefCount<TTrackStep> {
    TTrackStepPtr Parent;
    size_t Item;
    TDynBitMap Context;
};


class TMatchTrack {
protected:
    TVector<size_t> InputTrack;
    TVector<TDynBitMap> Contexts;
    size_t Start;
    size_t End;

public:
    TMatchTrack(size_t start, size_t end)
        : InputTrack()
        , Contexts()
        , Start(start)
        , End(end)
    {
    }

    TMatchTrack(const TVector<size_t>& track, size_t start, size_t end)
        : InputTrack(track)
        , Contexts()
        , Start(start)
        , End(end)
    {
        Y_ASSERT(Start <= End);
        Y_ASSERT(Start < InputTrack.size());
        Y_ASSERT(End <= InputTrack.size());
    }

    Y_FORCE_INLINE size_t GetStart() const {
        return Start;
    }

    Y_FORCE_INLINE size_t Size() const {
        return End - Start;
    }

    template<class TSymbol, class TAction>
    Y_FORCE_INLINE void ExtractMatched(const TInputTree<TSymbol>& input, const TAction& act) const {
        Y_ASSERT(!InputTrack.empty());
        input.ExtractSymbols(act, InputTrack, Start, End);
    }

    template<class TSymbol, class TAction>
    Y_FORCE_INLINE void ExtractMatched(const TVector<TSymbol>& inputVector, const TAction& act) const {
        Y_ASSERT(InputTrack.empty());
        Y_ASSERT(Start < inputVector.size());
        Y_ASSERT(End <= inputVector.size());
        ::ForEach(inputVector.begin() + Start, inputVector.begin() + End, act);
    }

    template<class TSymbol, class TAction>
    Y_FORCE_INLINE void ExtractMatchedSubRange(const TInputTree<TSymbol>& input, const std::pair<size_t, size_t>& subRange, const TAction& act) const {
        Y_ASSERT(!InputTrack.empty());
        Y_ASSERT(subRange.first <= subRange.second);
        Y_ASSERT(Start + subRange.first < InputTrack.size() && Start + subRange.second <= InputTrack.size());
        input.ExtractSymbols(act, InputTrack, Start + subRange.first, Start + subRange.second);
    }

    template<class TSymbol, class TAction>
    Y_FORCE_INLINE void ExtractMatchedSubRange(const TVector<TSymbol>& inputVector, const std::pair<size_t, size_t>& subRange, const TAction& act) const {
        Y_ASSERT(InputTrack.empty());
        Y_ASSERT(subRange.first <= subRange.second);
        Y_ASSERT(Start + subRange.first < inputVector.size() && Start + subRange.second <= inputVector.size());
        ::ForEach(inputVector.begin() + (Start + subRange.first), inputVector.begin() + (Start + subRange.second), act);
    }

    template<class TSymbol>
    Y_FORCE_INLINE const TSymbol& GetSymbolAt(const TInputTree<TSymbol>& input, size_t pos) const {
        Y_ASSERT(!InputTrack.empty());
        const TInputTree<TSymbol>* node = input.GetTrackNode(InputTrack, Start + pos);
        Y_ASSERT(nullptr != node);
        return node->GetSymbol();
    }

    template<class TSymbol>
    Y_FORCE_INLINE const TSymbol& GetSymbolAt(const TVector<TSymbol>& inputVector, size_t pos) const {
        Y_ASSERT(InputTrack.empty());
        Y_ASSERT(Start + pos < inputVector.size());
        return inputVector[Start + pos];
    }

    template<class TSymbol>
    Y_FORCE_INLINE typename TInputTree<TSymbol>::TSymbolIterator IterateMatched(const TInputTree<TSymbol>& input) const {
        Y_ASSERT(!InputTrack.empty());
        Y_ASSERT(Start < End);
        return input.Iterate(InputTrack, Start, End);
    }

    template<class TSymbol>
    Y_FORCE_INLINE TRangeIterator<const TSymbol, typename TVector<TSymbol>::const_iterator> IterateMatched(const TVector<TSymbol>& inputVector) const {
        Y_ASSERT(InputTrack.empty());
        Y_ASSERT(Start < inputVector.size());
        Y_ASSERT(End <= inputVector.size());
        return TRangeIterator<const TSymbol, typename TVector<TSymbol>::const_iterator>(inputVector.begin() + Start, inputVector.begin() + End);
    }

    Y_FORCE_INLINE const TVector<TDynBitMap>& GetContexts() const {
        return Contexts;
    }

    Y_FORCE_INLINE TVector<TDynBitMap>& GetContexts() {
        return Contexts;
    }
    void Swap(TMatchTrack& t) {
        DoSwap(InputTrack, t.InputTrack);
        DoSwap(Contexts, t.Contexts);
        DoSwap(Start, t.Start);
        DoSwap(End, t.End);
    }
};

class TIteratorBase {
protected:
    size_t Start;
    size_t End;

protected:
    TIteratorBase(size_t from = 0)
        : Start(from)
        , End(from)
    {
    }

    void Proceed(size_t inc) {
        End += inc;
    }

public:
    // Position relative to the matched sequence.
    // Used to form submatch coordinates
    Y_FORCE_INLINE size_t GetCurPos() const {
        return End - Start;
    }

    void Swap(TIteratorBase& it) {
        DoSwap(Start, it.Start);
        DoSwap(End, it.End);
    }
};

class TTrackIteratorBase: public TIteratorBase {
protected:
    TTrackStepPtr Track;
    TDynBitMap CurContex;

protected:
    TTrackIteratorBase(size_t from = 0)
        : TIteratorBase(from)
    {
    }

    void Proceed(size_t item = 0) {
        ++End;
        TTrackStepPtr nextStep(new TTrackStep());
        nextStep->Parent = Track;
        nextStep->Item = item;
        DoSwap(nextStep->Context, CurContex);
        DoSwap(nextStep, Track);
    }

    template <class TOp>
    void IterTrackSteps(TOp& op) const {
        TVector<const TTrackStep*> steps;
        steps.reserve(End);
        const TTrackStep* s = Track.Get();
        while (s) {
            steps.push_back(s);
            s = s->Parent.Get();
        }
        size_t n = 0;
        for (TVector<const TTrackStep*>::reverse_iterator iStep = steps.rbegin(); iStep != steps.rend(); ++iStep, ++n) {
            op(n, **iStep);
        }
    }

public:
    void Swap(TTrackIteratorBase& it) {
        TIteratorBase::Swap(it);
        DoSwap(Track, it.Track);
        DoSwap(CurContex, it.CurContex);
    }
};

template <class TSymbol>
class TVectorIterator: public TTrackIteratorBase {
private:
    const TVector<TSymbol>* Symbols;

private:
    struct TMatchTrackCreator: public TMatchTrack {
        TMatchTrackCreator(size_t start, size_t end)
            : TMatchTrack(start, end)
        {
            Contexts.reserve(end - start);
        }
        inline void operator() (size_t i, const TTrackStep& s) {
            Y_UNUSED(i);
            Contexts.push_back(s.Context);
        }
    };

private:
    // Disable default constructor
    TVectorIterator()
        : TTrackIteratorBase(0)
        , Symbols(NULL)
    {
    }

    inline TVectorIterator& Proceed() {
        TTrackIteratorBase::Proceed();
        return *this;
    }

public:
    TVectorIterator(const TVector<TSymbol>& symbols, size_t start = 0)
        : TTrackIteratorBase(start)
        , Symbols(&symbols)
    {
    }

    Y_FORCE_INLINE bool AtBegin() const {
        return 0 == Start && 0 == End;
    }

    Y_FORCE_INLINE bool AtEnd() const {
        return Symbols->size() == End;
    }

    inline TSymbol GetSymbol() const {
        Y_ASSERT(!AtEnd());
        return (*Symbols)[End];
    }

    template <class TLiteralTable>
    Y_FORCE_INLINE bool IsEqual(const TLiteralTable& lt, TLiteral l) {
        Y_ASSERT(!AtEnd());
        CurContex.Clear();
        return lt.IsEqual(l, GetSymbol(), CurContex);
    }

    inline size_t GetNextCount() const {
        return AtEnd() ? 0 : 1;
    }

    inline TVectorIterator GetNext(size_t n) const {
        Y_UNUSED(n);
        Y_ASSERT(n < GetNextCount());
        return TVectorIterator(*this).Proceed();
    }

    // Returns position range in the original sequence of input symbols
    Y_FORCE_INLINE std::pair<size_t, size_t> GetPosRange() const {
        return std::make_pair(Start, End);
    }

    void Swap(TVectorIterator& it) {
        TTrackIteratorBase::Swap(it);
        DoSwap(Symbols, it.Symbols);
    }

    TMatchTrack MakeTrack() const {
        TMatchTrackCreator track(Start, End);
        IterTrackSteps(track);
        return track;
    }
};

// Iterates the input and stores the track in the particular branch
// The track may start from any input node, not only from the root
template <class TSymbol>
class TInputTreeIterator : public TTrackIteratorBase {
private:
    const TInputTree<TSymbol>* Current;
    size_t CurrentOffset;
    std::pair<size_t, size_t> PosRange;

private:
    struct TMatchTrackCreator: public TMatchTrack {
        TMatchTrackCreator(size_t start, size_t end)
            : TMatchTrack(start, end)
        {
            InputTrack.reserve(end - start);
            Contexts.reserve(end - start);
        }
        inline void operator() (size_t i, const TTrackStep& s) {
            InputTrack.push_back(s.Item);
            if (i >= Start && i < End) {
                Contexts.push_back(s.Context);
            }
        }
    };

private:
    TInputTreeIterator& Proceed(const TInputTree<TSymbol>* s, size_t offset) {
        TTrackIteratorBase::Proceed(CurrentOffset);
        PosRange.second += Current->GetLength();
        Current = s;
        CurrentOffset = offset;
        return *this;
    }

public:
    TInputTreeIterator()
        : TTrackIteratorBase()
        , Current(NULL)
        , CurrentOffset(0)
        , PosRange(0, 0)
    {
    }

    TInputTreeIterator(const TInputTree<TSymbol>* start, size_t offset)
        : TTrackIteratorBase(0)
        , Current(start)
        , CurrentOffset(offset)
        , PosRange(0, 0)
    {
    }

    Y_FORCE_INLINE void StartMatch() {
        Start = End;
        PosRange.first = PosRange.second;
    }

    Y_FORCE_INLINE bool AtBegin() const {
        return 0 == PosRange.first && !Track;
    }

    Y_FORCE_INLINE bool AtEnd() const {
        return !Current;
    }

    TSymbol GetSymbol() const {
        Y_ASSERT(!AtEnd());
        return Current->GetSymbol();
    }

    template <class TLiteralTable>
    Y_FORCE_INLINE bool IsEqual(const TLiteralTable& lt, TLiteral l) {
        Y_ASSERT(!AtEnd());
        CurContex.Clear();
        return lt.IsEqual(l, GetSymbol(), CurContex);
    }

    inline size_t GetNextCount() const {
        if (Current) {
            return Current->GetNext().empty() ? 1 : Current->GetNext().size();
        }
        return 0;
    }

    inline TInputTreeIterator GetNext(size_t n) const {
        Y_ASSERT(n < GetNextCount());
        return Current->GetNext().empty()
            ? TInputTreeIterator(*this).Proceed(nullptr, 0)
            : TInputTreeIterator(*this).Proceed(&(Current->GetNext()[n]), n);
    }


    /// @param nodeFilterAction Action to be performed on next nodes, must return true for accepted ones.
    template <typename TNodeFilterAction>
    inline void Next(TVector<TInputTreeIterator>& iters, TNodeFilterAction& nodeFilterAction) const {
        if (Current) {
            const TVector<TInputTree<TSymbol>>& next = Current->GetNext();
            if (!next.empty()) {
                iters.reserve(iters.size() + next.size());
                for (size_t i = 0; i < next.size(); ++i) {
                    if (nodeFilterAction(next[i])) {
                        iters.push_back(*this);
                        iters.back().Proceed(&next[i], i);
                    }
                }
            } else {
                iters.push_back(*this);
                iters.back().Proceed(nullptr, 0);
            }
        }
    }

    // Returns position range in the original sequence of input symbols
    Y_FORCE_INLINE const std::pair<size_t, size_t>& GetPosRange() const {
        return PosRange;
    }

    void Swap(TInputTreeIterator& it) {
        TTrackIteratorBase::Swap(it);
        DoSwap(Current, it.Current);
        DoSwap(CurrentOffset, it.CurrentOffset);
        DoSwap(PosRange, it.PosRange);
    }

    TMatchTrack MakeTrack() const {
        TMatchTrackCreator track(Start, End);
        IterTrackSteps(track);
        return track;
    }
};

} // NRemorph
