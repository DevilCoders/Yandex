#pragma once

#include <kernel/remorph/common/gztfilter.h>
#include <kernel/remorph/common/gztiter.h>
#include <kernel/remorph/common/gztoccurrence.h>
#include <kernel/remorph/common/verbose.h>
#include <kernel/remorph/core/core.h>
#include <kernel/remorph/literal/logic_expr.h>
#include <kernel/remorph/literal/logic_filter.h>

#include <kernel/gazetteer/gazetteer.h>

#include <library/cpp/solve_ambig/occurrence.h>
#include <library/cpp/solve_ambig/rank.h>

#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NMatcher {

enum ESearchMethod {
    SM_SEARCH_ALL = 1,
    SM_MATCH_ALL = 2,
    SM_SEARCH = 3,
    SM_MATCH = 4,
};

enum EMatcherType {
    MT_REMORPH = 1,
    MT_TOKENLOGIC = 2,
    MT_CHAR = 3,
};

using NGztSupport::TGazetteerFilter;

class TMatcherBase: public TAtomicRefCount<TMatcherBase> {
protected:
    EMatcherType Type;
    TGazetteerFilter UsedGztItems;
    TGazetteerFilter DominantArticles;
    NLiteral::TLInstrVector Filter;  // Contains complex filter
    TGazetteerFilter FilterArticles; // Contains simple filter, which checks gzt articles only (substitutes Filter)
    bool ResolveGazetteerAmbiguity;
    NSolveAmbig::TRankMethod GazetteerRankMethod;

protected:
    TMatcherBase(EMatcherType type)
        : Type(type)
        , ResolveGazetteerAmbiguity(true)
        , GazetteerRankMethod(NSolveAmbig::DefaultRankMethod())
    {
    }

public:
    virtual ~TMatcherBase();

    virtual void CollectUsedGztItems(THashSet<TUtf16String>& gztItems) const = 0;
    virtual void SaveToStream(IOutputStream& out) const = 0;
    virtual void LoadFromFile(const TString& filePath, const NGzt::TGazetteer* gzt) = 0;
    virtual void LoadFromStream(IInputStream& in, bool signature) = 0;

    // Returns true if result has at least one article from the collection of required articles
    // If collection of required articles is empty than assume that any gazetteer result is acceptable
    template <class TGztResultIterator>
    bool FillGztOccurrences(TGztResultIterator& iter, NGztSupport::TOcc2GztItems<typename TGztResultIterator::TGztItem>& occ2Items) const {
        typedef NGztSupport::TGztItemCollection<typename TGztResultIterator::TGztItem> TGztItems;

        bool res = FilterArticles.Empty();
        for (; iter.Ok(); ++iter) {
            const bool dominant = iter.BelongsTo(DominantArticles);
            if (dominant || iter.BelongsTo(UsedGztItems)) {
                typename TGztResultIterator::TGztItem item = *iter;
                REPORT(VERBOSE, (dominant ? "Dominant: " : "Gzt: ") << iter.GetDebugString());
                NSolveAmbig::TOccurrence occ;
                occ.first = iter.GetStartPos();
                occ.second = iter.GetEndPos();
                TGztItems& val = occ2Items[occ];
                val.Items.push_back(item);
                val.IsDominant = val.IsDominant || dominant;
                res = res || iter.BelongsTo(FilterArticles);
            }
        }

        if (res)
            occ2Items.ResolveDominantAmbiguity(GazetteerRankMethod);

        return res;
    }

    void Init(const NGzt::TGazetteer* gzt);

    inline EMatcherType GetMatcherType() const {
        return Type;
    }

    inline const TGazetteerFilter& GetUsedGztItem() const {
        return UsedGztItems;
    }

    inline TGazetteerFilter& GetUsedGztItem() {
        return UsedGztItems;
    }

    inline const TGazetteerFilter& GetDominantArticles() const {
        return DominantArticles;
    }

    inline TGazetteerFilter& GetDominantArticles() {
        return DominantArticles;
    }

    inline void SetResolveGazetteerAmbiguity(bool resolve) {
        ResolveGazetteerAmbiguity = resolve;
    }

    inline bool GetResolveGazetteerAmbiguity() const {
        return ResolveGazetteerAmbiguity;
    }

    inline void SetGazetteerRankMethod(const NSolveAmbig::TRankMethod& gazetteerRankMethod) {
        GazetteerRankMethod = gazetteerRankMethod;
    }

    inline const NSolveAmbig::TRankMethod& GetGazetteerRankMethod() const {
        return GazetteerRankMethod;
    }

    void SetFilter(const TString& filter, const NGzt::TGazetteer* gzt = nullptr);

    /// @return false if input doesn't pass filters, true otherwise
    template <class TSymbolPtr>
    inline bool CreateInput(NRemorph::TInput<TSymbolPtr>& input, const TVector<TSymbolPtr>& symbols) const {
        CreateBaseInput(input, symbols);
        return FilterInput(input);
    }

    /// @return false if input doesn't pass filters, true otherwise
    template <class TSymbolPtr, class TGztResultIterator>
    inline bool CreateInput(NRemorph::TInput<TSymbolPtr>& input, const TVector<TSymbolPtr>& symbols, TGztResultIterator& gztIter) const {
        NGztSupport::TOcc2GztItems<typename TGztResultIterator::TGztItem> occ2Items;
        if (!FillGztOccurrences(gztIter, occ2Items)) {
            return false;
        }

        CreateBaseInput(input, symbols);

        bool resolveAmbiguity = ResolveGazetteerAmbiguity;
        NGztSupport::TSymbolHelper<TSymbolPtr> helper(symbols);
        if (!occ2Items.empty()) {
            if (!resolveAmbiguity && !occ2Items.ApplyAmbigOccurrences(input, helper)) {
                // If we are using all gazetteer results and the limit has been exceeded then force ambiguity resolving
                resolveAmbiguity = true;
                // Reset input to the single branch
                input.Fill(symbols.begin(), symbols.end());
            }

            if (resolveAmbiguity) {
                occ2Items.ResolveAmbiguity(GazetteerRankMethod);
                occ2Items.ApplyOccurrences(input, helper);
            }
        }

        return FilterInput(input);
    }

    /// @return false if input doesn't pass filters, true otherwise
    template <class TSymbolPtr>
    inline bool CreateInput(NRemorph::TInput<TSymbolPtr>& input, const TVector<TSymbolPtr>& symbols, const TGazetteer* gzt) const {
        if (!gzt) {
            return CreateInput(input, symbols);
        }

        NGztSupport::TGztArticleIter<TSymbolPtr> gztIter(*gzt, symbols);
        return CreateInput(input, symbols, gztIter);
    }

    // Updates the vector of symbols by the gazetter results. Multi-word articles substitutes corresponding
    // items in the vector. Returns true if gazetteer results contain at least one article from the collection
    // of required articles.
    template <class TSymbolPtr, class TGztResultIterator>
    bool ApplyGztResults(TVector<TSymbolPtr>& symbols, TGztResultIterator& iter) const {
        NGztSupport::TOcc2GztItems<typename TGztResultIterator::TGztItem> occ2Items;
        if (!FillGztOccurrences(iter, occ2Items))
            return false;

        if (!occ2Items.empty()) {
            occ2Items.ResolveAmbiguity(GazetteerRankMethod);
            NGztSupport::TSymbolHelper<TSymbolPtr> helper(symbols);
            occ2Items.ApplyOccurrences(symbols, helper);
        }
        return AcceptedByFilter(symbols);
    }

    template <class TSymbolPtr>
    inline size_t GetSearchableSize(const TVector<TSymbolPtr>& symbols) const {
        if (!FilterArticles.Empty())
            return NGztSupport::GetLastSymbolWithGazetteerItems(symbols, FilterArticles);
        if (!Filter.empty())
            return NLiteral::GetLastSymbolByLogicExp(symbols, Filter);
        return symbols.size();
    }

    template <class TSymbolPtr>
    inline bool ApplyGztResults(TVector<TSymbolPtr>& symbols, const TGazetteer* gzt) const {
        if (nullptr != gzt) {
            NGztSupport::TGztArticleIter<TSymbolPtr> iter(*gzt, symbols);
            return ApplyGztResults(symbols, iter);
        }
        return true;
    }

    template <class TSymbolPtr>
    inline bool FilterInput(NRemorph::TInput<TSymbolPtr>& input) const {
        if (!FilterArticles.Empty())
            return NGztSupport::FilterInputByGazetteerItems(input, FilterArticles);
        if (!Filter.empty())
            return NLiteral::FilterInputByLogicExp(input, Filter);
        return true;
    }

    template <class TSymbolPtr>
    inline bool AcceptedByFilter(const NRemorph::TInput<TSymbolPtr>& input) const {
        if (!FilterArticles.Empty())
            return NGztSupport::InputHasGazetteerItems(input, FilterArticles);
        if (!Filter.empty())
            return NLiteral::InputHasLogicExp(input, Filter);
        return true;
    }

    template <class TSymbolPtr>
    inline bool AcceptedByFilter(const TVector<TSymbolPtr>& input) const {
        return 0 != GetSearchableSize(input);
    }

protected:
    template <class TSymbolPtr>
    inline void EnsureFirstLastProperties(const TVector<TSymbolPtr>& symbols) const {
        if (symbols.empty()) {
            return;
        }

        symbols.front()->GetProperties().Set(NSymbol::PROP_FIRST);
        symbols.back()->GetProperties().Set(NSymbol::PROP_LAST);

    }

private:
    template <class TSymbolPtr>
    inline void CreateBaseInput(NRemorph::TInput<TSymbolPtr>& input, const TVector<TSymbolPtr>& symbols) const {
        EnsureFirstLastProperties(symbols);
        input.Fill(symbols.begin(), symbols.end());
    }
};

} // NMatcher
