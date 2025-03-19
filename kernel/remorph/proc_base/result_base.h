#pragma once

#include <kernel/remorph/core/core.h>

#include <library/cpp/solve_ambig/occurrence.h>

#include <util/stream/str.h>
#include <util/stream/trace.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <utility>

namespace NMatcher {

struct TResultBase: public TSimpleRefCount<TResultBase> {
    TString RuleName;
    double Priority;
    // Positions of the matched region relative to the original sequence.
    NRemorph::TSubmatch WholeExpr;
    // Path in the input
    NRemorph::TMatchTrack MatchTrack;

    TResultBase(const TString& rule, double priority, const std::pair<size_t, size_t>& origRange, const NRemorph::TMatchTrack& track)
        : RuleName(rule)
        , Priority(priority)
        , WholeExpr(origRange)
        , MatchTrack(track)
    {
    }

    TResultBase(const TString& rule, const std::pair<size_t, size_t>& origRange, const NRemorph::TMatchTrack& track)
        : RuleName(rule)
        , Priority(1.0)
        , WholeExpr(origRange)
        , MatchTrack(track)
    {
    }

    virtual ~TResultBase() {
    }

    virtual void GetNamedRanges(NRemorph::TNamedSubmatches& ranges) const = 0;

    inline size_t GetMatchedCount() const {
        return MatchTrack.Size();
    }

    inline double GetWeight() const {
        Y_ASSERT(MatchTrack.Size() > 0);
        // The longer sequence of source objects is matched - the better weight.
        // The longer symbols are matched (shorter track) - the better weight.
        return Priority * double(WholeExpr.Size()) / double(MatchTrack.Size());
    }

    template <class TInputSource>
    double GetRangeWeight(const TInputSource& inputSource, const std::pair<size_t, size_t>& subRange, bool contextWeight = true) const {
        typedef typename TInputSource::value_type TSymbol;
        TVector<TSymbol> symbols;
        TVector<TDynBitMap> contexts;
        ExtractMatched(inputSource, subRange, symbols, contexts);
        double w = contextWeight
            ? double(WholeExpr.Size()) / double(symbols.back()->GetSourcePos().second - symbols.front()->GetSourcePos().first)
            : 1.0;

        for (size_t i = 0; i < symbols.size(); ++i) {
            w *= symbols[i]->CalcWeight(contexts[i]);
        }
        return w;
    }

    // TInputSource is either TVector or NRemorph::TInput of input symbols
    template <class TInputSource>
    inline typename TInputSource::value_type GetMatchedSymbol(const TInputSource& inputSource, size_t pos) const {
        return MatchTrack.GetSymbolAt(inputSource, pos);
    }

    // TInputSource is either TVector or NRemorph::TInput of input symbols
    template <class TInputSource>
    inline typename TInputSource::value_type GetMatchedSymbol(const TInputSource& inputSource, size_t pos, TDynBitMap& ctx) const {
        ctx = MatchTrack.GetContexts()[pos];
        return MatchTrack.GetSymbolAt(inputSource, pos);
    }

    // TInputSource is either TVector or NRemorph::TInput of input symbols
    template <class TInputSource, class TSymbolPtr>
    inline void ExtractMatched(const TInputSource& inputSource, TVector<TSymbolPtr>& symbols) const {
        MatchTrack.ExtractMatched(inputSource, NRemorph::TPtrPusher<TSymbolPtr>(symbols));
    }

    // TInputSource is either TVector or NRemorph::TInput of input symbols
    template <class TInputSource, class TSymbolPtr>
    inline void ExtractMatched(const TInputSource& inputSource, TVector<TSymbolPtr>& symbols,
        TVector<TDynBitMap>& contexts) const {

        MatchTrack.ExtractMatched(inputSource, NRemorph::TPtrPusher<TSymbolPtr>(symbols));
        contexts.assign(MatchTrack.GetContexts().begin(), MatchTrack.GetContexts().end());
    }

    // TInputSource is either TVector or NRemorph::TInput of input symbols
    template <class TInputSource, class TSymbolPtr>
    inline void ExtractMatched(const TInputSource& inputSource, const std::pair<size_t, size_t>& subRange,
        TVector<TSymbolPtr>& symbols) const {
        MatchTrack.ExtractMatchedSubRange(inputSource, subRange, NRemorph::TPtrPusher<TSymbolPtr>(symbols));
    }

    // TInputSource is either TVector or NRemorph::TInput of input symbols
    template <class TInputSource, class TSymbolPtr>
    inline void ExtractMatched(const TInputSource& inputSource, const std::pair<size_t, size_t>& subRange,
        TVector<TSymbolPtr>& symbols, TVector<TDynBitMap>& contexts) const {
        MatchTrack.ExtractMatchedSubRange(inputSource, subRange, NRemorph::TPtrPusher<TSymbolPtr>(symbols));
        contexts.assign(MatchTrack.GetContexts().begin() + subRange.first, MatchTrack.GetContexts().begin() + subRange.second);
    }

    // TInputSource is either TVector or NRemorph::TInput of input symbols
    template <class TInputSource>
    inline NRemorph::TSubmatch GetWholeExprInOriginalSequence(const TInputSource& inputSource) const {
        return NRemorph::TSubmatch(
            MatchTrack.GetSymbolAt(inputSource, 0)->GetSourcePos().first,
            MatchTrack.GetSymbolAt(inputSource, MatchTrack.Size() - 1)->GetSourcePos().second
        );
    }

    // TInputSource is either TVector or NRemorph::TInput of input symbols
    template <class TInputSource>
    inline NRemorph::TSubmatch SubmatchToOriginal(const TInputSource& inputSource, const std::pair<size_t, size_t>& matchRange) const {
        return NRemorph::TSubmatch(
            MatchTrack.GetSymbolAt(inputSource, matchRange.first)->GetSourcePos().first,
            MatchTrack.GetSymbolAt(inputSource, matchRange.second - 1)->GetSourcePos().second
        );
    }
};

} // NMatcher

namespace NSolveAmbig {

template <>
struct TOccurrenceTraits<NMatcher::TResultBase> {
    inline static TStringBuf GetId(const NMatcher::TResultBase& r) {
        return r.RuleName;
    }
    inline static size_t GetCoverage(const NMatcher::TResultBase& r) {
        return r.WholeExpr.Size();
    }
    inline static size_t GetStart(const NMatcher::TResultBase& r) {
        return r.WholeExpr.first;
    }
    inline static size_t GetStop(const NMatcher::TResultBase& r) {
        return r.WholeExpr.second;
    }
    inline static double GetWeight(const NMatcher::TResultBase& r) {
        return r.GetWeight();
    }
};

} // NSolveAmbig
