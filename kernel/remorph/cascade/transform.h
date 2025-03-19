#pragma once

#include "cascade_base.h"
#include "remorph_item.h"
#include "tokenlogic_item.h"
#include "char_item.h"
#include "transform_params.h"

#include <kernel/remorph/common/verbose.h>
#include <kernel/remorph/core/core.h>
#include <kernel/remorph/literal/logic_filter.h>
#include <kernel/remorph/proc_base/matcher_base.h>

#include <util/charset/wide.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <utility>

namespace NCascade {

namespace NPrivate {

struct TCascadeResultHelper {
    Y_FORCE_INLINE static bool NeedCreate(const std::pair<size_t, size_t>&) {
        return true;
    }

    template <class TSymbolPtr>
    Y_FORCE_INLINE static void Set(const std::pair<size_t, size_t>&, const TVector<TSymbolPtr>&) {
    }

    template <class TSymbolPtr>
    Y_FORCE_INLINE static void Create(TVector<TSymbolPtr>& res, const std::pair<size_t, size_t>&, const TVector<TSymbolPtr>& items) {
        res.assign(items.begin(), items.end());
    }
};

} // NPrivate

// Apply given cascade item and insert results to the occurrence collection
template <class TSymbolPtr, class TCascadeItem>
inline NGztSupport::TOcc2GztItems<TSymbolPtr> MatcherTransform(const TCascadeItem& cascadeItem, TTransformParams<TSymbolPtr>& params) {
    typedef typename TSymbolPtr::TValueType TSymbol;
    typedef typename TCascadeItem::TResult TResult;
    //typedef typename TCascadeItem::TResultPtr TResultPtr;
    typedef typename TCascadeItem::TResults TResults;

    NGztSupport::TOcc2GztItems<TSymbolPtr> occ2Items;
    TResults results;
    TVector<TSymbolPtr> matchSymbols;
    TVector<TDynBitMap> matchContexts;

    const bool isDominant = params.DominantArticles.Has(cascadeItem.Article);
    cascadeItem.SearchAllCascaded(params.Input, results);

    for (typename TResults::const_iterator iRes = results.begin(); iRes != results.end(); ++iRes) {
        const TResult& res = **iRes;
        NRemorph::TSubmatch matchRange = cascadeItem.GetMatchRange(res);
        NRemorph::TSubmatch srcRange = res.SubmatchToOriginal(params.Input, matchRange);

        const size_t main = cascadeItem.GetMainWord(res);
        // The more context is used - the better rule
        Y_ASSERT(srcRange.Size() > 0);
        const double weight = res.GetWeight() * res.GetRangeWeight(params.Input, matchRange);

        matchSymbols.clear();
        matchContexts.clear();
        res.ExtractMatched(params.Input, matchRange, matchSymbols, matchContexts);

        TSymbolPtr newSymbol(new TSymbol(matchSymbols.begin(), matchSymbols.end(), matchContexts, main));
        // Add articles from head as sticky.
        // This allows correctly doing gazetteer agreement between cascaded symbols.
        newSymbol->AddGztArticles(*matchSymbols[main], matchContexts[main], true);
        newSymbol->AddGztArticle(cascadeItem.Article);
        newSymbol->SetRuleName(res.RuleName);
        newSymbol->SetWholeSourcePos(res.WholeExpr);
        newSymbol->SetWeight(weight);
        cascadeItem.GetNamedSubRanges(res, matchRange, newSymbol->GetNamedSubRanges());

        NGztSupport::TGztItemCollection<TSymbolPtr>& coll = occ2Items[srcRange];
        bool found = false;
        if (coll.Items.empty()) {
            coll.Occ.Weight = weight;
        } else {
            if (coll.Occ.Weight < weight)
                coll.Occ.Weight = (float)weight;
            for (typename TVector<TSymbolPtr>::iterator iItem = coll.Items.begin(); iItem != coll.Items.end(); ++iItem) {
                const TSymbolPtr& s = *iItem;
                if (s->GetHead() == newSymbol->GetHead() && s->GetChildren() == newSymbol->GetChildren() && s->GetNamedSubRanges() == newSymbol->GetNamedSubRanges()) {
                    if (s->GetWeight() < weight) {
                        DoSwap(*iItem, newSymbol);
                    }
                    found = true;
                    break;
                }
            }
        }
        if (!found)
            coll.Items.push_back(newSymbol.Get());

        REPORT(DEBUG, "Cascade \"" << WideToUTF8(cascadeItem.ArticleTitle)
            << "\", src=[" << srcRange.first << "," << srcRange.second
            << "), w=" << weight << ", unique=" << !found << ", "
            << res.ToDebugString(GetVerbosityLevel(), params.Input)
        );
        coll.IsDominant = coll.IsDominant || isDominant;
    }
    return occ2Items;
}

// Apply all cascades recursively and insert results into the input
template <class TSymbolPtr>
inline void Transfrom(const TCascadeBase& cascades, TTransformParams<TSymbolPtr>& params) {
    NGztSupport::TOcc2GztItems<TSymbolPtr> occ2Items;
    TVector<NGzt::TArticleId> processedChildren;
    for (TCascadeItems::const_iterator iCascade = cascades.SubCascades.begin(); iCascade != cascades.SubCascades.end(); ++iCascade) {
        const TCascadeItem* cascade = iCascade->Get();
        if (cascade->GetMatcher().AcceptedByFilter(params.Input)) {
            if (!params.ProcessedCascades.has(cascade->Article.GetId())) {
                processedChildren.push_back(cascade->Article.GetId());
                if (!cascade->SubCascades.empty()) {
                    Transfrom(*cascade, params);
                }
                NGztSupport::TOcc2GztItems<TSymbolPtr> localItems;
                switch (cascade->Type) {
                case NMatcher::MT_REMORPH:
                    REPORT(DETAIL, "Running remorph cascade: " << WideToUTF8(cascade->ArticleTitle));
                    localItems = MatcherTransform(*static_cast<const TRemorphCascadeItem*>(cascade), params);
                    REPORT(DETAIL, "Finishing remorph cascade: " << WideToUTF8(cascade->ArticleTitle));
                    break;
                case NMatcher::MT_TOKENLOGIC:
                    REPORT(DETAIL, "Running tokenlogic cascade: " << WideToUTF8(cascade->ArticleTitle));
                    localItems = MatcherTransform(*static_cast<const TTokenLogicCascadeItem*>(cascade), params);
                    REPORT(DETAIL, "Finishing tokenlogic cascade: " << WideToUTF8(cascade->ArticleTitle));
                    break;
                case NMatcher::MT_CHAR:
                    REPORT(DETAIL, "Running char cascade: " << WideToUTF8(cascade->ArticleTitle));
                    localItems = MatcherTransform(*static_cast<const TCharCascadeItem*>(cascade), params);
                    REPORT(DETAIL, "Finishing char cascade: " << WideToUTF8(cascade->ArticleTitle));
                    break;
                default:
                    throw yexception() << "Unsupported cascade type";
                }
                // Use separate occurrence collection for each transformer in order to not allow
                // override results of one cascade by another one.
                if (!localItems.empty()) {
                    if (occ2Items.empty())
                        DoSwap(occ2Items, localItems);
                    else
                        occ2Items.Merge(localItems);
                }
            } else {
                REPORT(DETAIL, "Skipping already processed cascade: " << WideToUTF8(cascade->ArticleTitle));
            }
        } else {
            REPORT(DETAIL, "Skipping cascade by filter: " << WideToUTF8(cascade->ArticleTitle));
        }
    }
    bool markProcessed = true;
    if (!occ2Items.empty()) {
        NPrivate::TCascadeResultHelper helper;
        bool resolveAmbig = cascades.ResolveCascadeAmbiguity;
        if (!resolveAmbig) {
            occ2Items.ResolveDominantAmbiguity(cascades.CascadeRankMethod);
            NRemorph::TInput<TSymbolPtr> inputCopy(params.Input);
            if (!occ2Items.ApplyAmbigOccurrences(params.Input, helper)) {
                // Restore original input
                DoSwap(inputCopy, params.Input);
                resolveAmbig = true;
            }
        }
        if (resolveAmbig) {
            markProcessed = false;
            occ2Items.ResolveAmbiguity(cascades.CascadeRankMethod);
            occ2Items.ResolveEqualsByPriority();
            if (Y_UNLIKELY(::GetVerbosityLevel() >= TRACE_DEBUG)) {
                for (typename NGztSupport::TOcc2GztItems<TSymbolPtr>::const_iterator i = occ2Items.begin(); i != occ2Items.end(); ++i) {
                    REPORT(DEBUG, "Selected \"" << i->Items.front()->GetRuleName()
                        << "\", src=[" << i->Occ.first << "," << i->Occ.second
                        << "), w=" << i->Occ.Weight
                    );
                }
            }
            occ2Items.ApplyOccurrences(params.Input, helper);
        }
    }

    if (markProcessed) {
        // If all children results are inserted into the input then remember all child cascades as processed
        // Don't mark processed cascades in case of resolving ambiguity
        params.ProcessedCascades.insert_unique(processedChildren.begin(), processedChildren.end());
    }
}

} // NCascade
