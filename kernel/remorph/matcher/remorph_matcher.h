#pragma once

#include "match_result.h"

#include <kernel/gazetteer/gazetteer.h>
#include <kernel/remorph/common/conv.h>
#include <kernel/remorph/common/gztfilter.h>
#include <kernel/remorph/common/verbose.h>
#include <kernel/remorph/core/core.h>
#include <kernel/remorph/input/input_symbol_util.h>
#include <kernel/remorph/literal/agreement.h>
#include <kernel/remorph/literal/literal_table.h>
#include <kernel/remorph/proc_base/matcher_base.h>

#include <util/charset/wide.h>
#include <util/datetime/base.h>
#include <util/generic/bitmap.h>
#include <util/generic/ptr.h>
#include <util/generic/hash_set.h>
#include <util/stream/output.h>
#include <util/stream/trace.h>
#include <util/string/vector.h>
#include <utility>

namespace NReMorph {

using namespace NSymbol;
using NGztSupport::TGazetteerFilter;

class TMatcher;
typedef TIntrusivePtr<TMatcher> TMatcherPtr;

class TMatcher: public NMatcher::TMatcherBase {
public:
    typedef TMatchResult TResult;
    typedef TMatchResultPtr TResultPtr;
    typedef TMatchResults TResults;

private:
    struct TNextNodeDepthFilter {
        const size_t& MinimalDepth;

        explicit TNextNodeDepthFilter(const size_t& minimalDepth)
            : MinimalDepth(minimalDepth)
        {
        }

        template <typename TSymbolPtr>
        inline bool operator ()(const NRemorph::TInput<TSymbolPtr>& node) const {
            return node.HasDepth(MinimalDepth);
        }
    };

    struct TNextNodeDepthAndAcceptanceFilter: public TNextNodeDepthFilter {
        explicit TNextNodeDepthAndAcceptanceFilter(const size_t& minimalDepth)
            : TNextNodeDepthFilter(minimalDepth)
        {
        }

        template <typename TSymbolPtr>
        inline bool operator ()(const NRemorph::TInput<TSymbolPtr>& node) const {
            return node.IsAccepted() && TNextNodeDepthFilter::operator ()(node);
        }
    };


private:
    NLiteral::TLiteralTablePtr LiteralTable;
    TVector<std::pair<TString, double>> RuleNames;
    TVector<TUtf16String> ExternalArticles;
    NRemorph::TDFAPtr Dfa;
    size_t MaxSearchStates; // Maximum number of search states when traversing DFA
    size_t MinimalPathLength;
    TNextNodeDepthFilter NextNodeDepthFilter;
    TNextNodeDepthAndAcceptanceFilter NextNodeDepthAndAcceptanceFilter;

private:
    template <class TSymbolIterator>
    struct TAgreementMatcher {
        const NLiteral::TLiteralTablePtr& LiteralTable;
        TSymbolIterator SymbolIter;
        NRemorph::TRangeIterator<TDynBitMap, TVector<TDynBitMap>::iterator> CtxIter;
        NLiteral::TAgreementContext Context;
        bool Shift;

        TAgreementMatcher(const NLiteral::TLiteralTablePtr& lt, const TSymbolIterator& iter, TVector<TDynBitMap>& ctxs)
            : LiteralTable(lt)
            , SymbolIter(iter)
            , CtxIter(ctxs.begin(), ctxs.end())
            , Shift(false)
        {
        }

        inline bool operator() (NRemorph::TLiteral l) {
            if (l.IsMarker()) {
                // In case of MARKER literal we should check agreement for previous and current symbols
                // This condition checks the agreement for the previous symbol
                // The case, when MARKER appears at the beginning, is verified by the Shift checking
                if (Shift && SymbolIter.Ok() && !LiteralTable->CheckAgreements(l, SymbolIter->Get(), *CtxIter, Context))
                    return true;
            }
            // Use delayed iterator shift
            if (Shift) {
                ++CtxIter;
                ++SymbolIter;
            }
            Shift = !l.IsMarker();
            Y_ASSERT(SymbolIter.Ok() || l.IsMarker());
            return SymbolIter.Ok() ? !LiteralTable->CheckAgreements(l, SymbolIter->Get(), *CtxIter, Context) : false;
        }
    };

    template <class TSymbolIterator>
    Y_FORCE_INLINE static TAgreementMatcher<TSymbolIterator> CreateAgreementMatcher(const NLiteral::TLiteralTablePtr& lt,
        const TSymbolIterator& iter, TVector<TDynBitMap>& ctxs) {
        return TAgreementMatcher<TSymbolIterator>(lt, iter, ctxs);
    }

    struct TMatchIterChecker {
        template <class TInputIterator>
        Y_FORCE_INLINE bool operator()(const TInputIterator& iter) const {
            return iter.AtEnd();
        }
    };

    struct TSearchIterChecker {
        template <class TInputIterator>
        Y_FORCE_INLINE bool operator()(const TInputIterator&) const {
            return true;
        }
    };

    template <class TInputSymbols, class TResultHolder, class TIterChecker>
    class TResultCollector {
    private:
        const TMatcher& Matcher;
        const TInputSymbols& Symbols;
        TResultHolder& ResultHolder;
        const TIterChecker IterChecker;

    public:
        TResultCollector(const TMatcher& m, const TInputSymbols& s, TResultHolder& resHolder)
            : Matcher(m)
            , Symbols(s)
            , ResultHolder(resHolder)
            , IterChecker()
        {
        }

        // Controls the search stop point
        Y_FORCE_INLINE operator bool() const {
            return ResultHolder.AcceptMore();
        }

        // Functor for checking match result
        template <class TInputIterator>
        void operator()(const NRemorph::TMatchState<TInputIterator>& state) {
            if (!IterChecker(state.GetIter())) {
                return;
            }
            NRemorph::TMatchTrack track = state.GetIter().MakeTrack();
            if (!state.PostMatch(CreateAgreementMatcher(Matcher.LiteralTable, track.IterateMatched(Symbols), track.GetContexts()))) {
                return;
            }
            TVector<NRemorph::TMatchInfoPtr> matchInfos = state.CreateMatchInfos();
            // A little optimization: in most cases we have only single result,
            // so avoid additional memory allocation
            if (matchInfos.size() == 1) {
                TMatchResultPtr res = new TMatchResult(matchInfos.front(),
                    Matcher.RuleNames[matchInfos.front()->MatchedId], state.GetIter().GetPosRange(), track);
                ResultHolder.Put(res);
            } else {
                for (TVector<NRemorph::TMatchInfoPtr>::const_iterator i = matchInfos.begin();
                    i != matchInfos.end() && ResultHolder.AcceptMore(); ++i) {

                    TMatchResultPtr res = new TMatchResult(*i, Matcher.RuleNames[i->Get()->MatchedId],
                        state.GetIter().GetPosRange(), track);
                    ResultHolder.Put(res);
                }
            }
        }
    };

private:
    inline void WarnIfLostSearchResults(const bool full) const {
        if (Y_UNLIKELY(!full)) {
            REPORT(WARN, "The limit " << int(MaxSearchStates)
                << " for number of search states has been exceeded. Some of search results could be lost");
        }
    }

    template <class TSymbolPtr>
    inline bool IsAcceptable(const NRemorph::TInput<TSymbolPtr>& input, bool useAcceptFlag = true) const {
        if (useAcceptFlag && !input.IsAccepted())
            return false;
        // Accept any input if DFA can match singles
        if (Dfa->MinimalPathLength < 2)
            return true;
        // Leading dummy head has zero length. Increase checked length for it
        return input.GetLength() ? input.HasDepth(Dfa->MinimalPathLength) : input.HasDepth(Dfa->MinimalPathLength + 1);
    }

    template <class TSymbolPtr>
    inline bool IsAcceptable(const TVector<TSymbolPtr>& inputSequence) const {
        // Accept any input if DFA can match singles
        return Dfa->MinimalPathLength < 2 || inputSequence.size() >= Dfa->MinimalPathLength;
    }

    template <class TResultHolder, class TSymbolPtr>
    void InternalMatch(TResultHolder& resHolder, const NRemorph::TInput<TSymbolPtr>& input, NRemorph::OperationCounter* opcnt) const {
        if (!IsAcceptable(input))
            return;

        NSymbol::ClearMatchCache(input);
        TResultCollector<NRemorph::TInput<TSymbolPtr>, TResultHolder, TMatchIterChecker> collector(*this, input, resHolder);

        bool full = true;
        for (size_t i = 0; i < input.GetNext().size() && collector; ++i) {
            if (IsAcceptable(input.GetNext()[i])) {
                full = NRemorph::SearchFrom(*LiteralTable, *Dfa,
                    NRemorph::TInputTreeIterator<TSymbolPtr>(&input.GetNext()[i], i),
                    collector, MaxSearchStates, opcnt) && full;
            }
        }
        WarnIfLostSearchResults(full);
    }

    template <class TResultHolder, class TSymbolPtr>
    void InternalMatch(TResultHolder& resHolder, const TVector<TSymbolPtr>& inputSequence, NRemorph::OperationCounter* opcnt) const {
        EnsureFirstLastProperties(inputSequence);

        if (!IsAcceptable(inputSequence))
            return;

        if (!AcceptedByFilter(inputSequence))
            return;

        NSymbol::ClearMatchCache(inputSequence);
        TResultCollector<TVector<TSymbolPtr>, TResultHolder, TMatchIterChecker> collector(*this, inputSequence, resHolder);

        const bool full = NRemorph::SearchFrom(*LiteralTable, *Dfa,
            NRemorph::TVectorIterator<TSymbolPtr>(inputSequence),
            collector, MaxSearchStates, opcnt);
        WarnIfLostSearchResults(full);
    }

    template <class TResultHolder, class TSymbolPtr>
    void InternalSearch(TResultHolder& resHolder, const NRemorph::TInput<TSymbolPtr>& input, NRemorph::OperationCounter* opcnt, bool useAcceptFlag = true) const {
        if (!IsAcceptable(input, useAcceptFlag)) {
            return;
        }

        NSymbol::ClearMatchCache(input);
        TResultCollector<NRemorph::TInput<TSymbolPtr>, TResultHolder, TSearchIterChecker> collector(*this, input, resHolder);
        TVector<NRemorph::TInputTreeIterator<TSymbolPtr>> iters;

        for (size_t i = 0; i < input.GetNext().size(); ++i) {
            if (IsAcceptable(input.GetNext()[i], useAcceptFlag))
                iters.push_back(NRemorph::TInputTreeIterator<TSymbolPtr>(&input.GetNext()[i], i));
        }
        bool full = true;
        const bool onlyFromStart = 0 != (Dfa->Flags & NRemorph::DFAFLG_STARTS_WITH_ANCHOR);
        while (!iters.empty() && collector) {
            NRemorph::TInputTreeIterator<TSymbolPtr> from = iters.back();
            iters.pop_back();
            from.StartMatch();
            if (Y_LIKELY(!onlyFromStart)) {
                // Next() performs filtering by the branch length and Accepted flag.
                // It doesn't return iterators for nodes, which are not accepted by a filter
                // or which start too short branches.
                from.Next(iters, useAcceptFlag ? NextNodeDepthAndAcceptanceFilter : NextNodeDepthFilter);
            }
            full = NRemorph::SearchFrom(*LiteralTable, *Dfa, from, collector, MaxSearchStates, opcnt) && full;
        }
        WarnIfLostSearchResults(full);
    }

    template <class TResultHolder, class TSymbolPtr>
    void InternalSearch(TResultHolder& resHolder, const TVector<TSymbolPtr>& inputSequence, NRemorph::OperationCounter* opcnt) const {
        EnsureFirstLastProperties(inputSequence);

        if (!IsAcceptable(inputSequence))
            return;

        const size_t searchableSize = GetSearchableSize(inputSequence);
        if (0 == searchableSize)
            return;

        NSymbol::ClearMatchCache(inputSequence);
        TResultCollector<TVector<TSymbolPtr>, TResultHolder, TSearchIterChecker> collector(*this, inputSequence, resHolder);

        bool full = true;
        size_t minDepth = Dfa->MinimalPathLength < 2 ? 0 : Dfa->MinimalPathLength - 1;
        if (0 != (Dfa->Flags & NRemorph::DFAFLG_STARTS_WITH_ANCHOR))
            minDepth = Max(minDepth, inputSequence.size() - 1);
        for (size_t i = 0; i + minDepth < inputSequence.size() && i < searchableSize && collector; ++i) {
            full = NRemorph::SearchFrom(*LiteralTable, *Dfa,
                NRemorph::TVectorIterator<TSymbolPtr>(inputSequence, i),
                collector, MaxSearchStates, opcnt) && full;
        }
        WarnIfLostSearchResults(full);
    }

protected:
    TMatcher()
        : NMatcher::TMatcherBase(NMatcher::MT_REMORPH)
        , MaxSearchStates(1000)
        , MinimalPathLength(0) // updated on dfa load
        , NextNodeDepthFilter(MinimalPathLength)
        , NextNodeDepthAndAcceptanceFilter(MinimalPathLength)
    {
    }

    void LoadFromFile(const TString& filePath, const NGzt::TGazetteer* gzt) override;
    void LoadFromStream(IInputStream& in, bool signature) override;

    void ParseFromString(const TString& rules, const NGzt::TGazetteer* gzt) {
        TStringInput stream(rules);
        ParseFromArcadiaStream(stream, gzt);
    }
    void ParseFromArcadiaStream(IInputStream& rules, const NGzt::TGazetteer* gzt);

public:
    void SetMaxSearchStates(size_t states) {
        MaxSearchStates = states;
    }

    size_t GetMaxSearchStates() const {
        return MaxSearchStates;
    }

    void CollectUsedGztItems(THashSet<TUtf16String>& gztItems) const override;
    void SaveToStream(IOutputStream& out) const override;

    // Load remorph from the specified file. The Gazetteer reference is required only for article names validation.
    // The matcher doesn't keep the reference to the gazetteer
    static TMatcherPtr Load(const TString& filePath, const NGzt::TGazetteer* gzt);
    // Load compiled remorph from the stream
    static TMatcherPtr Load(IInputStream& in, const NGzt::TGazetteer* gzt);
    // Load compiled remorph from the stream
    static TMatcherPtr Load(IInputStream& in);
    // Parse remorph from the specified string
    static TMatcherPtr Parse(const TString& rules, const NGzt::TGazetteer* gzt = nullptr);
    // Parse remorph from the specified stream
    static TMatcherPtr Parse(IInputStream& rules, const NGzt::TGazetteer* gzt = nullptr);

public:
    template <class TInputSource>
    inline TMatchResultPtr Match(const TInputSource& inputSource, NRemorph::OperationCounter* opcnt = nullptr) const {
        NRemorph::TSingleResultHolder<TMatchResultPtr> holder;
        InternalMatch(holder, inputSource, opcnt);
        return holder.Result;
    }

    template <class TInputSource>
    inline void MatchAll(const TInputSource& inputSource, TMatchResults& results, NRemorph::OperationCounter* opcnt = nullptr) const {
        NRemorph::TMultiResultHolder<TMatchResults> holder(results);
        InternalMatch(holder, inputSource, opcnt);
    }

    template <class TInputSource>
    inline TMatchResultPtr Search(const TInputSource& inputSource, NRemorph::OperationCounter* opcnt = nullptr) const {
        NRemorph::TSingleResultHolder<TMatchResultPtr> holder;
        InternalSearch(holder, inputSource, opcnt);
        return holder.Result;
    }

    template <class TInputSource>
    inline void SearchAll(const TInputSource& inputSource, TMatchResults& results, NRemorph::OperationCounter* opcnt = nullptr) const {
        NRemorph::TMultiResultHolder<TMatchResults> holder(results);
        InternalSearch(holder, inputSource, opcnt);
    }

    template <class TSymbolPtr>
    inline void SearchAllCascaded(const NRemorph::TInput<TSymbolPtr>& input, TMatchResults& results, NRemorph::OperationCounter* opcnt = nullptr) const {
        NRemorph::TMultiResultHolder<TMatchResults> holder(results);
        InternalSearch(holder, input, opcnt, false);
    }
};

} // NReMorph
