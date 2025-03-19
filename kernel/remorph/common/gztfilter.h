#pragma once

#include "conv.h"

#include <kernel/gazetteer/gazetteer.h>
#include <kernel/remorph/core/core.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/charset/wide.h>

namespace NGztSupport {

// The support class, which holds the mixed collection of gzt type names and article titles,
// and allows checking is an article belongs to this collection or not.
class TGazetteerFilter {
private:
    THashSet<const NGzt::TDescriptor*> Types;
    THashSet<ui32> ArticleIDs;

private:
    template <class TStr>
    bool TryAddAsType(const NGzt::TGazetteer& gzt, const TStr& name) {
        const NGzt::TDescriptor* descr = gzt.ProtoPool().FindMessageTypeByName(ToUTF8(name));
        if (nullptr != descr) {
            Types.insert(descr);
            return true;
        }
        return false;
    }

    template <class TStr>
    bool TryAddAsArticle(const NGzt::TGazetteer& gzt, const TStr& name) {
        NGzt::TArticlePtr article = gzt.LoadArticle(ToWide(name));
        if (!article.IsNull()) {
            ArticleIDs.insert(article.GetId());
            return true;
        }
        return false;
    }

public:
    // Creates empty collection
    TGazetteerFilter() {
    }

    // Creates collection for the specified gazetteer instance and specified set of names
    // The constructor can raise an exception if some name has no associated type or article
    template <class TCollection>
    TGazetteerFilter(const NGzt::TGazetteer& gzt, const TCollection& coll) {
        Add(gzt, coll);
    }

    // Add the set of names belonging to the passed gazetteer instance to the collection
    // The method can raise an exception if some name has no associated type or article
    template <class TCollection>
    void Add(const NGzt::TGazetteer& gzt, const TCollection& coll) {
        for (typename TCollection::const_iterator i = coll.begin(); i != coll.end(); ++i) {
            AddItem(gzt, *i);
        }
    }

    template <class TStr>
    void AddItem(const NGzt::TGazetteer& gzt, const TStr& name) {
        // Some names are present as both type and article. So, try to add both.
        bool added = TryAddAsType(gzt, name);
        added = TryAddAsArticle(gzt, name) || added;
        if (!added) {
            throw yexception() << "Cannot find Gazetteer type or article with the '" << name << "' name";
        }
    }

    Y_FORCE_INLINE void Add(const TGazetteerFilter& other) {
        Types.insert(other.Types.begin(), other.Types.end());
        ArticleIDs.insert(other.ArticleIDs.begin(), other.ArticleIDs.end());
    }

    Y_FORCE_INLINE void Add(const TArticlePtr& article) {
        ArticleIDs.insert(article.GetId());
    }

    Y_FORCE_INLINE void Add(const NGzt::TDescriptor* descr) {
        Types.insert(descr);
    }

    Y_FORCE_INLINE bool Has(const NGzt::TArticlePtr& a) const {
        return !a.IsNull() && (Types.contains(a.GetType()) || ArticleIDs.contains(a.GetId()));
    }

    Y_FORCE_INLINE bool Has(const NGzt::TDescriptor* d) const {
        return Types.contains(d);
    }

    Y_FORCE_INLINE bool Has(const NGzt::TArticleId& id) const {
        return ArticleIDs.contains(id);
    }

    template <class TIter>
    bool HasAnyType(const TIter& start, const TIter& end) const {
        for (TIter i = start; i != end; ++i) {
            if (Types.contains(*i))
                return true;
        }
        return false;
    }

    template <class TIter>
    bool HasAnyArticle(const TIter& start, const TIter& end) const {
        for (TIter i = start; i != end; ++i) {
            if (ArticleIDs.contains(*i))
                return true;
        }
        return false;
    }

    bool Has(const TGazetteerFilter& other) const {
        // Iterate over minimal collection
        bool res = Types.size() < other.Types.size()
            ? other.HasAnyType(Types.begin(), Types.end())
            : HasAnyType(other.Types.begin(), other.Types.end());
        if (!res) {
            res = ArticleIDs.size() < other.ArticleIDs.size()
                ? other.HasAnyArticle(ArticleIDs.begin(), ArticleIDs.end())
                : HasAnyArticle(other.ArticleIDs.begin(), other.ArticleIDs.end());
        }
        return res;
    }

    Y_FORCE_INLINE bool Empty() const {
        return Types.empty() && ArticleIDs.empty();
    }

    Y_FORCE_INLINE void Clear() {
        Types.clear();
        ArticleIDs.clear();
    }

    void Swap(TGazetteerFilter& other) {
        DoSwap(Types, other.Types);
        DoSwap(ArticleIDs, other.ArticleIDs);
    }
};

namespace NPrivate {

// For using with TInput::Filter
template <class TSymbolPtr>
struct TGazetteerInputFilter {
    const TGazetteerFilter& NameSet;

    TGazetteerInputFilter(const TGazetteerFilter& s)
        : NameSet(s)
    {
    }

    inline bool operator() (const TSymbolPtr& s) const {
        for (TVector<TArticlePtr>::const_iterator i = s->GetGztArticles().begin(); i != s->GetGztArticles().end(); ++i) {
            if (NameSet.Has(*i))
                return true;
        }
        return false;
    }
};

// For using with TInput::TraverseSymbols
template <class TSymbolPtr>
struct TGazetteerChecker {
    const TGazetteerFilter& NameSet;
    bool Res;

    explicit TGazetteerChecker(const TGazetteerFilter& s)
        : NameSet(s)
        , Res(false)
    {
    }

    inline bool operator() (const TSymbolPtr& s) {
        for (TVector<TArticlePtr>::const_iterator i = s->GetGztArticles().begin(); i != s->GetGztArticles().end(); ++i) {
            if (NameSet.Has(*i)) {
                Res = true;
                break;
            }
        }
        // Stop traversing when have found symbol with articles
        return !Res;
    }
};

} // NPrivate

// Remains only branches, which have at least one input symbol with any gazetteer article from the specified collection,
// and removes all other branches. Returns true if input has at least one sub-branch after removal.
template <class TSymbolPtr>
Y_FORCE_INLINE bool FilterInputByGazetteerItems(NRemorph::TInput<TSymbolPtr>& input, const TGazetteerFilter& filter) {
    return filter.Empty() || input.Filter(NPrivate::TGazetteerInputFilter<TSymbolPtr>(filter));
}

// Return offset to the last symbol, which conforms the supplied gazetteer filter, or zero if no such symbol
template <class TSymbolPtr>
inline size_t GetLastSymbolWithGazetteerItems(const TVector<TSymbolPtr>& vec, const TGazetteerFilter& filter) {
    const size_t off = filter.Empty()
        ? vec.size()
        : std::distance(vec.begin(), FindIf(vec.rbegin(), vec.rend(), NPrivate::TGazetteerInputFilter<TSymbolPtr>(filter)).base());
    return off;
}

// Returns true if input has at least one input symbol with any gazetteer article from the specified collection
template <class TSymbolPtr>
inline bool InputHasGazetteerItems(const NRemorph::TInput<TSymbolPtr>& input, const TGazetteerFilter& filter) {
    NPrivate::TGazetteerChecker<TSymbolPtr> gazetteerChecker(filter);
    return filter.Empty() || (input.TraverseSymbols(gazetteerChecker), gazetteerChecker.Res);
}

} // NGztSupport
