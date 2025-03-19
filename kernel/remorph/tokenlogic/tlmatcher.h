#pragma once

#include "rule.h"
#include "tlresult.h"

#include <kernel/remorph/core/core.h>
#include <kernel/remorph/input/input_symbol.h>
#include <kernel/remorph/literal/literal_table.h>
#include <kernel/remorph/proc_base/matcher_base.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NTokenLogic {

using namespace NSymbol;
using namespace NLiteral;

class TMatcher;
typedef TIntrusivePtr<TMatcher> TMatcherPtr;

class TMatcher: public NMatcher::TMatcherBase {
public:
    typedef TTokenLogicResult TResult;
    typedef TTokenLogicResultPtr TResultPtr;
    typedef TTokenLogicResults TResults;

private:
    NLiteral::TLiteralTablePtr LiteralTable;
    TVector<NPrivate::TRule> Rules;
    TVector<TUtf16String> ExternalArticles;

private:
    struct TMatchChecker {
        template <class TContext>
        Y_FORCE_INLINE bool operator()(const TContext& ctx) const {
            return 0 == ctx.OtherCount || ctx.HasAny;
        }
    };
    struct TSearchChecker {
        Y_FORCE_INLINE bool operator()(const NPrivate::TMatchContext&) const {
            return true;
        }
    };

    struct TResultMaker {
        const TVector<size_t>& Track;

        TResultMaker(const TVector<size_t>& track)
            : Track(track)
        {
        }

        Y_FORCE_INLINE TResultPtr operator()(const NPrivate::TRule& rule,
            const NPrivate::TMatchContext& ctx, const std::pair<size_t, size_t>& origRange) const {
            return new TResult(rule.Name, rule.Weight, origRange, ctx, Track);
        }
    };

    struct TVectorResultMaker {
        Y_FORCE_INLINE TResultPtr operator()(const NPrivate::TRule& rule,
            const NPrivate::TMatchContext& ctx, const std::pair<size_t, size_t>& origRange) const {
            return new TResult(rule.Name, rule.Weight, origRange, ctx);
        }
    };

    template <class TResultHolder, class TChecker>
    struct TBranchMatcher {
        const TMatcher& Matcher;
        TResultHolder& Res;
        const TChecker& Checker;

        TBranchMatcher(const TMatcher& matcher, TResultHolder& res, const TChecker& checker)
            : Matcher(matcher)
            , Res(res)
            , Checker(checker)
        {
        }

        template <class TSymbolPtr>
        bool operator()(const TVector<TSymbolPtr>& curBranch, const TVector<size_t>& track) const {
            Matcher.InternalMatch(Res, curBranch, Checker, TResultMaker(track));
            return Res.AcceptMore();
        }
    };

private:
    template <class TResultHolder, class TSymbolPtr, class TChecker, class TResultMaker>
    inline void InternalMatch(TResultHolder& res, const TVector<TSymbolPtr>& symbols,
        const TChecker& check, const TResultMaker& resMaker) const {
        NPrivate::TRuleContext<TSymbolPtr> ctx(symbols, *LiteralTable);
        for (size_t i = 0; i < Rules.size() && res.AcceptMore(); ++i) {
            if (Rules[i].Matches(ctx) && check(ctx)) {
                Y_ASSERT(ctx.Range.first < ctx.Range.second);
                Y_ASSERT(ctx.Range.first < symbols.size());
                Y_ASSERT(ctx.Range.second <= symbols.size());
                std::pair<size_t, size_t> origRange;
                origRange.first = symbols[ctx.Range.first]->GetSourcePos().first;
                origRange.second = symbols[ctx.Range.second - 1]->GetSourcePos().second;
                res.Put(resMaker(Rules[i], ctx, origRange));
            }
        }
    }

    template <class TResultHolder, class TSymbolPtr, class TChecker>
    inline void InternalMatch(TResultHolder& res, const NRemorph::TInput<TSymbolPtr>& symbols, const TChecker& check) const {
        NSymbol::ClearMatchCache(symbols);
        TBranchMatcher<TResultHolder, TChecker> branchMatcher(*this, res, check);
        symbols.TraverseBranches(branchMatcher);
    }

    template <class TResultHolder, class TSymbolPtr, class TChecker>
    inline void InternalMatch(TResultHolder& res, const TVector<TSymbolPtr>& symbols, const TChecker& check) const {
        EnsureFirstLastProperties(symbols);

        if (!AcceptedByFilter(symbols))
            return;
        NSymbol::ClearMatchCache(symbols);
        InternalMatch(res, symbols, check, TVectorResultMaker());
    }

protected:
    TMatcher()
        : NMatcher::TMatcherBase(NMatcher::MT_TOKENLOGIC)
    {
    }

    void LoadFromFile(const TString& filePath, const NGzt::TGazetteer* gzt) override;
    void LoadFromStream(IInputStream& in, bool signature) override;

    void ParseFromString(const TString& rules, const NGzt::TGazetteer* gzt);

public:
    void CollectUsedGztItems(THashSet<TUtf16String>& gztItems) const override;
    void SaveToStream(IOutputStream& out) const override;

    static TMatcherPtr Load(const TString& filePath, const NGzt::TGazetteer* gzt);
    static TMatcherPtr Load(IInputStream& in, const NGzt::TGazetteer* gzt);
    static TMatcherPtr Load(IInputStream& in);
    static TMatcherPtr Parse(const TString& rules, const NGzt::TGazetteer* gzt = nullptr);

public:
    template <class TInputSource>
    inline TResultPtr Match(const TInputSource& symbols, NRemorph::OperationCounter* = nullptr) const {
        NRemorph::TSingleResultHolder<TResultPtr> resHolder;
        InternalMatch(resHolder, symbols, TMatchChecker());
        return resHolder.Result;
    }

    template <class TInputSource>
    inline void MatchAll(const TInputSource& symbols, TResults& results, NRemorph::OperationCounter* = nullptr) const {
        NRemorph::TMultiResultHolder<TResults> resHolder(results);
        InternalMatch(resHolder, symbols, TMatchChecker());
    }

    template <class TInputSource>
    inline TResultPtr Search(const TInputSource& symbols, NRemorph::OperationCounter* = nullptr) const {
        NRemorph::TSingleResultHolder<TResultPtr> resHolder;
        InternalMatch(resHolder, symbols, TSearchChecker());
        return resHolder.Result;
    }

    template <class TInputSource>
    inline void SearchAll(const TInputSource& symbols, TResults& results, NRemorph::OperationCounter* = nullptr) const {
        NRemorph::TMultiResultHolder<TResults> resHolder(results);
        InternalMatch(resHolder, symbols, TSearchChecker());
    }

    template <class TSymbolPtr>
    inline void SearchAllCascaded(const NRemorph::TInput<TSymbolPtr>& input, TResults& results, NRemorph::OperationCounter* = nullptr) const {
        SearchAll(input, results);
    }
};

} // NTokenLogic
