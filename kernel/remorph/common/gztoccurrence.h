#pragma once

#include "article_util.h"
#include "verbose.h"

#include <kernel/gazetteer/gztarticle.h>

#include <library/cpp/solve_ambig/solve_ambiguity.h>
#include <library/cpp/solve_ambig/rank.h>
#include <library/cpp/solve_ambig/occurrence.h>

#include <util/generic/ptr.h>
#include <util/generic/bitmap.h>
#include <util/generic/vector.h>
#include <util/generic/utility.h>
#include <library/cpp/containers/sorted_vector/sorted_vector.h>

namespace NGztSupport {

template <typename TGztItem>
struct TGztItemTraits {
    template <class TSymbol>
    static void AddGztItem(TSymbol& s, const TGztItem& item);

    template <class TSymbol>
    static void AddGztItems(TSymbol& s, const TVector<TGztItem>& items);

    static const NGzt::TArticlePtr& ToArticle(const TGztItem& item);
};

template <typename TGztItem>
struct TGztItemCollection {
    NSolveAmbig::TOccurrence Occ;
    TVector<TGztItem> Items;
    bool IsDominant;

    struct TKey {
        Y_FORCE_INLINE const std::pair<size_t, size_t>& operator ()(const TGztItemCollection& item) const {
            return item.Occ;
        }
    };

    TGztItemCollection()
        : IsDominant(false)
    {
    }

    TGztItemCollection(const std::pair<size_t, size_t>& occ)
        : Occ(occ)
        , IsDominant(false)
    {
    }
};

template <typename TGztItem>
struct TOcc2GztItems : public NSorted::TSimpleSet<TGztItemCollection<TGztItem>, std::pair<size_t, size_t>, typename TGztItemCollection<TGztItem>::TKey> {
    typedef NSorted::TSimpleSet<TGztItemCollection<TGztItem>, std::pair<size_t, size_t>, typename TGztItemCollection<TGztItem>::TKey> TBase;

    void ResolveDominantAmbiguity(const NSolveAmbig::TRankMethod& rankMethod = NSolveAmbig::DefaultRankMethod()) {
        TVector<NSolveAmbig::TOccurrence> occurrences;
        for (typename TBase::const_iterator iOcc = TBase::begin(); iOcc != TBase::end(); ++iOcc) {
            if (iOcc->IsDominant) {
                occurrences.push_back(iOcc->Occ);
            }
        }
        if (!occurrences.empty()) {
            TVector<NSolveAmbig::TOccurrence> dropped;
            NSolveAmbig::SolveAmbiguity(occurrences, dropped, rankMethod);
            for (TVector<NSolveAmbig::TOccurrence>::const_iterator i = dropped.begin(); i != dropped.end(); ++i) {
                this->operator[](*i).IsDominant = false;
            }
        }
    }

    void ResolveAmbiguity(const NSolveAmbig::TRankMethod& rankMethod = NSolveAmbig::DefaultRankMethod()) {
        TVector<NSolveAmbig::TOccurrence> occurrences;
        occurrences.reserve(TBase::size());
        for (typename TBase::const_iterator iOcc = TBase::begin(); iOcc != TBase::end(); ++iOcc) {
            occurrences.push_back(iOcc->Occ);
        }

        if (!occurrences.empty()) {
            TVector<NSolveAmbig::TOccurrence> dropped;
            NSolveAmbig::SolveAmbiguity(occurrences, dropped, rankMethod);
            for (TVector<NSolveAmbig::TOccurrence>::const_iterator i = dropped.begin(); i != dropped.end(); ++i) {
                TBase::erase(*i);
            }
        }
    }

    void ResolveEqualsByPriority() {
        for (typename TBase::iterator iOcc = TBase::begin(); iOcc != TBase::end(); ++iOcc) {
            TGztItemCollection<TGztItem>& coll = *iOcc;
            if (coll.Items.size() > 1) {
                size_t maxWeightIndex = 0;
                for (size_t i = 1 ; i < coll.Items.size(); ++i) {
                    if (coll.Items[maxWeightIndex]->GetWeight() < coll.Items[i]->GetWeight())
                        maxWeightIndex = i;
                }
                if (maxWeightIndex > 0) {
                    DoSwap(coll.Items[0], coll.Items[maxWeightIndex]);
                }
                coll.Items.resize(1);
            }
        }
    }

    // Applies ambiguous occurrences. Returns false if number of created branches exceed the limit
    template <class TSymbolPtr, class TSymbolHelper>
    bool ApplyAmbigOccurrences(NRemorph::TInput<TSymbolPtr>& input, const TSymbolHelper& helper) const {
        // Apply dominant occurrences first
        TVector<TSymbolPtr> multiSymbols;
        for (typename TBase::const_iterator iOcc = TBase::begin(); iOcc != TBase::end(); ++iOcc) {
            if (helper.NeedCreate(iOcc->Occ) && iOcc->IsDominant) {
                const NSolveAmbig::TOccurrence& occ = iOcc->Occ;
                multiSymbols.clear();
                helper.Create(multiSymbols, occ, iOcc->Items);
                if (!multiSymbols.empty()) {
                    input.CreateBranches(occ.first, occ.second, multiSymbols, true);
                }
            }
        }

        // Apply remaining multi-symbol occurrences
        for (typename TBase::const_iterator iOcc = TBase::begin(); iOcc != TBase::end(); ++iOcc) {
            if (helper.NeedCreate(iOcc->Occ) && !iOcc->IsDominant) {
                const NSolveAmbig::TOccurrence& occ = iOcc->Occ;
                multiSymbols.clear();
                helper.Create(multiSymbols, occ, iOcc->Items);
                if (!multiSymbols.empty()) {
                    input.CreateBranches(occ.first, occ.second, multiSymbols, false);
                    if (Y_UNLIKELY(input.ExceedsBranchLimit())) {
                        REPORT(WARN, "The limit for count of branches has been exceeded!!!");
                        return false;
                    }
                }
            }
        }

        // Apply single-item results only if the input doesn't exceed the branch limit
        for (typename TBase::const_iterator iOcc = TBase::begin(); iOcc != TBase::end(); ++iOcc) {
            if (!helper.NeedCreate(iOcc->Occ)) {
                helper.Set(iOcc->Occ, iOcc->Items);
            }
        }

        return true;
    }

    // Applies unambiguous occurrences
    template <class TSymbolPtr, class TSymbolHelper>
    void ApplyOccurrences(NRemorph::TInput<TSymbolPtr>& input, const TSymbolHelper& helper) const {
        TVector<TSymbolPtr> multiSymbols;
        for (typename TBase::const_iterator iOcc = TBase::begin(); iOcc != TBase::end(); ++iOcc) {
            const NSolveAmbig::TOccurrence& occ = iOcc->Occ;
            if (helper.NeedCreate(occ)) {
                multiSymbols.clear();
                helper.Create(multiSymbols, occ, iOcc->Items);
                if (!multiSymbols.empty()) {
                    input.CreateBranches(occ.first, occ.second, multiSymbols, true);
                }
            } else {
                helper.Set(occ, iOcc->Items);
            }
        }
        // We can still exceed the limit even after resolving ambiguity,
        // because each occurrence item can contain multiple symbols
        // (the same rule raised twice in different contexts)
        if (Y_UNLIKELY(input.ExceedsBranchLimit())) {
            // Just warn about the fact
            REPORT(WARN, "The limit for count of branches has been exceeded!!!");
        }
    }

    // Applies unambiguous occurrences to the vector
    template <class TSymbolPtr, class TSymbolHelper>
    void ApplyOccurrences(TVector<TSymbolPtr>& symbols, const TSymbolHelper& helper) const {
        TVector<TSymbolPtr> res;
        TVector<TSymbolPtr> multiSymbols;

        typename TBase::const_iterator iOcc = TBase::begin();
        for (size_t i = 0; i < symbols.size();) {
            bool added = false;
            if (iOcc != TBase::end() && iOcc->Occ.first == i) {
                if (helper.NeedCreate(iOcc->Occ)) {
                    res.push_back(helper.Create(iOcc->Occ, iOcc->Items));
                } else {
                    helper.Set(iOcc->Occ, iOcc->Items);
                    for (size_t s = iOcc->Occ.first; s < iOcc->Occ.second; ++s) {
                        res.push_back(symbols[s]);
                    }
                }
                i = iOcc->Occ.second;
                ++iOcc;
                added = true;
            }
            if (!added) {
                res.push_back(symbols[i]);
                ++i;
            }
        }
        DoSwap(res, symbols);
    }

    void Merge(const TOcc2GztItems& items) {
        for (typename TOcc2GztItems::const_iterator iOcc = items.begin(); iOcc != items.end(); ++iOcc) {
            std::pair<typename TOcc2GztItems::iterator, bool> res = TBase::Insert(*iOcc);
            if (!res.second) { // Already exists
                TGztItemCollection<TGztItem>& myColl = *res.first;
                myColl.Occ.Weight = Max(myColl.Occ.Weight, iOcc->Occ.Weight);
                myColl.Items.insert(myColl.Items.end(), iOcc->Items.begin(), iOcc->Items.end());
                myColl.IsDominant = myColl.IsDominant || iOcc->IsDominant;
            }
        }
    }
};


template <class TSymbolPtr>
struct TSymbolHelper {
    typedef typename TSymbolPtr::TValueType TSymbol;

    const TVector<TSymbolPtr>& Symbols;

    TSymbolHelper(const TVector<TSymbolPtr>& symbols)
        : Symbols(symbols)
    {
    }

    Y_FORCE_INLINE static bool NeedCreate(const std::pair<size_t, size_t>& range) {
        return range.second - range.first > 1;
    }

    template <typename TGztItem>
    Y_FORCE_INLINE void Set(const std::pair<size_t, size_t>& range, const TVector<TGztItem>& items) const {
        TGztItemTraits<TGztItem>::AddGztItems(*Symbols[range.first], items);
    }

    // Creates symbols for specified range and set of articles.
    // If articles have different main word values then separate symbol is created for each such article
    // Other articles with unspecified main word are added to all created symbols.
    // Articles are added in the same order as they are specified.
    template <typename TGztItem>
    void Create(TVector<TSymbolPtr>& res, const std::pair<size_t, size_t>& range, const TVector<TGztItem>& items) const {

        NSorted::TSimpleMap<ui32, TSymbolPtr> symbolsWithHeads;
        TVector<TGztItem> commonItems;
        commonItems.reserve(items.size());
        for (typename TVector<TGztItem>::const_iterator iGzt = items.begin(); iGzt != items.end(); ++iGzt) {
            ui32 mainWordIndex = 0;
            // Create a separate symbol for articles with explicitly specified different main words
            // Add other articles to all created symbols
            if (GetMainGztWord(TGztItemTraits<TGztItem>::ToArticle(*iGzt), mainWordIndex)) {
                if (mainWordIndex >= range.second - range.first)
                    mainWordIndex = range.second - range.first - 1;

                // Create separate input symbols for articles with different main words
                typename NSorted::TSimpleMap<ui32, TSymbolPtr>::iterator iW = symbolsWithHeads.find(mainWordIndex);
                if (iW != symbolsWithHeads.end()) {
                    // Add item with explicit main word
                    TGztItemTraits<TGztItem>::AddGztItem(*(iW->second), *iGzt);
                } else {
                    TSymbolPtr inputSymbol(new TSymbol(Symbols.begin() + range.first, Symbols.begin() + range.second,
                        TVector<TDynBitMap>(), mainWordIndex));
                    // Add already processed common items
                    TGztItemTraits<TGztItem>::AddGztItems(*inputSymbol, commonItems);
                    // Add item with explicit main word
                    TGztItemTraits<TGztItem>::AddGztItem(*inputSymbol, *iGzt);
                    symbolsWithHeads[mainWordIndex] = inputSymbol;
                    res.push_back(inputSymbol.Get());
                }
            } else {
                commonItems.push_back(*iGzt);
                for (typename TVector<TSymbolPtr>::iterator iS = res.begin(); iS != res.end(); ++iS) {
                    TGztItemTraits<TGztItem>::AddGztItem(**iS, *iGzt);
                }
            }
        }
        // Process case when there is no article with explicit main word. Create a single symbol with all articles
        if (res.empty()) {
            TSymbolPtr inputSymbol(new TSymbol(Symbols.begin() + range.first, Symbols.begin() + range.second,
                TVector<TDynBitMap>(), 0));
            TGztItemTraits<TGztItem>::AddGztItems(*inputSymbol, items);
            res.push_back(inputSymbol.Get());
        }
    }

    // Creates single symbol for specified range and set of articles.
    // The main word is taken from a first article, which has it set
    template <typename TGztItem>
    TSymbolPtr Create(const std::pair<size_t, size_t>& range, const TVector<TGztItem>& items) const {
        ui32 mainWordIndex = 0;
        for (typename TVector<TGztItem>::const_iterator iGzt = items.begin(); iGzt != items.end(); ++iGzt) {
            ui32 main = 0;
            if (GetMainGztWord(TGztItemTraits<TGztItem>::ToArticle(*iGzt), main)
                && main < range.second - range.first) {

                mainWordIndex = main;
            }
        }

        TSymbolPtr inputSymbol(new TSymbol(Symbols.begin() + range.first, Symbols.begin() + range.second,
            TVector<TDynBitMap>(), mainWordIndex));
        TGztItemTraits<TGztItem>::AddGztItems(*inputSymbol, items);
        return inputSymbol;
    }
};

} // NGztSupport
