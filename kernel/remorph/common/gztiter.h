#pragma once

#include "gztfilter.h"
#include "gztoccurrence.h"
#include "conv.h"

#include <kernel/gazetteer/gazetteer.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <utility>

namespace NGztSupport {

template <>
struct TGztItemTraits<TArticlePtr> {
    template <class TSymbol>
    Y_FORCE_INLINE static void AddGztItem(TSymbol& s, const TArticlePtr& item) {
        s.AddGztArticle(item);
    }

    template <class TSymbol>
    Y_FORCE_INLINE static void AddGztItems(TSymbol& s, const TVector<TArticlePtr>& items) {
        s.AddGztArticles(items);
    }

    Y_FORCE_INLINE static const TArticlePtr& ToArticle(const TArticlePtr& item) {
        return item;
    }
};


template <class TSymbolPtr>
class TGztArticleIter : protected TArticleIter<TVector<TSymbolPtr>> {
private:
    typedef TArticleIter<TVector<TSymbolPtr>> TBase;

    const TGazetteer* Gazetteer;
    const TVector<TSymbolPtr>* Symbols;
    const NGzt::TDescriptor* CurrentArticleDescriptor;
    TArticlePtr CurrentArticle;

protected:
    inline const TArticlePtr& GetArticle() {
        Y_ASSERT(Ok());
        if (CurrentArticle.IsNull())
            CurrentArticle = Gazetteer->GetArticle(*this);
        return CurrentArticle;
    }

    Y_FORCE_INLINE NGzt::TArticleId GetArticleId() const {
        Y_ASSERT(Ok());
        return TBase::operator*();
    }

    inline const NGzt::TDescriptor* GetArticleDescriptor() {
        if (nullptr == CurrentArticleDescriptor)
            CurrentArticleDescriptor = Gazetteer->GetDescriptor(*this);
        return CurrentArticleDescriptor;
    }

public:
    typedef TArticlePtr TGztItem;

public:
    TGztArticleIter(const TGazetteer& gzt, const TVector<TSymbolPtr>& symbols)
        : Gazetteer(&gzt)
        , Symbols(&symbols)
        , CurrentArticleDescriptor(nullptr)
    {
        Gazetteer->IterArticles(*Symbols, this);
    }

    using TBase::Ok;

    Y_FORCE_INLINE const TArticlePtr& operator*() {
        return GetArticle();
    }

    Y_FORCE_INLINE TGztArticleIter& operator++() {
        TBase::operator++();
        CurrentArticle = TArticlePtr();
        CurrentArticleDescriptor = nullptr;
        return *this;
    }

    Y_FORCE_INLINE bool BelongsTo(const TGazetteerFilter& filter) {
        Y_ASSERT(TBase::Ok());
        return filter.Has(GetArticleId()) || filter.Has(GetArticleDescriptor());
    }

    Y_FORCE_INLINE size_t GetStartPos() const {
        Y_ASSERT(TBase::Ok());
        return TBase::GetWords().FirstWordIndex();
    }

    Y_FORCE_INLINE size_t GetEndPos() const {
        Y_ASSERT(TBase::Ok());
        return TBase::GetWords().FirstWordIndex() + TBase::GetWords().Size();
    }

    Y_FORCE_INLINE void Reset() {
        Gazetteer->IterArticles(*Symbols, this);
    }

    inline TString GetDebugString() {
        Y_ASSERT(TBase::Ok());
        return TString().append(GetArticleDescriptor()->name())
            .append("[")
            .append(WideToUTF8(Gazetteer->ArticlePool().FindArticleNameByOffset(GetArticleId())))
            .append("]: ")
            .append(ToUTF8(TBase::DebugFoundPhrase()));
    }
};

} // NGztSupport
