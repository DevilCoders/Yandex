#pragma once

#include "properties.h"

#include <kernel/gazetteer/gazetteer.h>
#include <kernel/remorph/core/core.h>

#include <kernel/lemmer/dictlib/gleiche.h>
#include <kernel/lemmer/dictlib/grambitset.h>

#include <library/cpp/containers/sorted_vector/sorted_vector.h>
#include <util/generic/algorithm.h>
#include <util/generic/ptr.h>
#include <util/generic/bitmap.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/charset/wide.h>

namespace NSymbol {

class TInputSymbol;

struct TGztLemma {
    TUtf16String Text;
    TVector<NGzt::TArticlePtr> Articles;

    struct TKey {
        Y_FORCE_INLINE const TUtf16String& operator() (const TGztLemma& l) const {
            return l.Text;
        }
    };

    TGztLemma() {
    }

    explicit TGztLemma(const TUtf16String& s)
        : Text(s)
    {
    }

    Y_FORCE_INLINE TGztLemma& Add(const TVector<NGzt::TArticlePtr>& v) {
        Articles.insert(Articles.end(), v.begin(), v.end());
        return *this;
    }

    Y_FORCE_INLINE TGztLemma& operator +=(const TGztLemma& a) {
        Text += a.Text;
        Articles.insert(Articles.end(), a.Articles.begin(), a.Articles.end());
        return *this;
    }

    Y_FORCE_INLINE TGztLemma& operator +=(wchar16 c) {
        Text.append(c);
        return *this;
    }
};

typedef NSorted::TSimpleSet<TGztLemma, TUtf16String, TGztLemma::TKey> TGztLemmas;

struct TInputSymbolLength {
    template <class TSymbolPtr>
    Y_FORCE_INLINE size_t operator()(const TSymbolPtr& s) const {
        return s->GetSourcePos().second - s->GetSourcePos().first;
    }
};

template <class TSymbol>
struct TInputSymbolFactory {
    template <class TSource>
    Y_FORCE_INLINE TIntrusivePtr<TSymbol> operator()(size_t pos, const TSource& s) const {
        return new TSymbol(pos, s);
    }
};

namespace NPrivate {
    struct TPtrExtractor {
        template <class TSymbol>
        Y_FORCE_INLINE TInputSymbol* operator()(const TIntrusivePtr<TSymbol>& ptr) const {
            return ptr.Get();
        }
    };
}

template <class TSymbol>
inline void TransformToBase(TVector<TIntrusivePtr<TInputSymbol>>& res, const TVector<TIntrusivePtr<TSymbol>>& v) {
    res.resize(v.size());
    std::transform(v.begin(), v.end(), res.begin(), NPrivate::TPtrExtractor());
}

namespace NPrivate {
    struct TCacheCleaner {
        template <class TSymbolPtr>
        Y_FORCE_INLINE bool operator() (const TSymbolPtr& s) const {
            s->ClearMatchResults();
            return true; // Traverse all symbols
        }
    };
}

template <class TSymbolPtr>
inline void ClearMatchCache(const NRemorph::TInput<TSymbolPtr>& input) {
    NPrivate::TCacheCleaner cacheCleaner;
    input.TraverseSymbols(cacheCleaner);
}

template <class TSymbolPtr>
inline void ClearMatchCache(const TVector<TSymbolPtr>& input) {
    std::for_each(input.begin(), input.end(), NPrivate::TCacheCleaner());
}

template<class TIter, class Extractor>
inline TUtf16String JoinInputSymbolText(TIter begin, TIter end, Extractor extract) {
    TUtf16String result;
    if (begin != end) {
        result.append(extract(*begin));
        for (++begin; begin != end; ++begin) {
            if (begin->Get()->GetProperties().Test(PROP_SPACE_BEFORE)) {
                result.append(' ');
            }
            result.append(extract(*begin));
        }
    }
    return result;
}

struct TTextExtractor {
    template <class TSymbolPtr>
    Y_FORCE_INLINE TUtf16String operator() (const TSymbolPtr& s) const {
        return s.Get() != nullptr ? s->GetText() : TUtf16String();
    }
};

struct TNormalizedTextExtractor {
    template <class TSymbolPtr>
    Y_FORCE_INLINE TUtf16String operator() (const TSymbolPtr& s) const {
        return s.Get() != nullptr ? s->GetNormalizedText() : TUtf16String();
    }
};

struct TDebugTextExtractor {
    template <class TSymbolPtr>
    Y_FORCE_INLINE TUtf16String operator() (const TSymbolPtr& s) const {
        return s.Get() != nullptr ? UTF8ToWide(s->ToDebugString()) : TUtf16String();
    }
};

template <class TSymbolPtr>
inline TString ToString(const TVector<TSymbolPtr>& symbols) {
    return WideToUTF8(JoinInputSymbolText(symbols.begin(), symbols.end(), TTextExtractor()));
}

template <class TIter>
inline TString ToString(TIter start, TIter end) {
    return WideToUTF8(JoinInputSymbolText(start, end, TTextExtractor()));
}

template <class TSymbolPtr>
inline TUtf16String ToWtroku(const TVector<TSymbolPtr>& symbols) {
    return JoinInputSymbolText(symbols.begin(), symbols.end(), TTextExtractor());
}

template <class TIter>
inline TUtf16String ToWtroku(TIter start, TIter end) {
    return JoinInputSymbolText(start, end, TTextExtractor());
}

template <class TSymbolPtr>
inline TGztLemma GetGazetteerForm(const TVector<TSymbolPtr>& symbols, const TVector<TDynBitMap>& contexts) {
    Y_ASSERT(symbols.size() == contexts.size());
    TGztLemma res;
    if (!symbols.empty()) {
        res = symbols.front()->GetGazetteerForm(contexts.front());
        for (size_t pos = 1; pos < symbols.size(); ++pos) {
            if (symbols[pos]->GetProperties().Test(PROP_SPACE_BEFORE)) {
                res += wchar16(' ');
            }
            res += symbols[pos]->GetGazetteerForm(contexts[pos]);
        }
    }
    return res;
}

template <class TSymbolPtr>
inline TGztLemmas GetAllGazetteerForms(const TVector<TSymbolPtr>& symbols, const TVector<TDynBitMap>& contexts) {
    Y_ASSERT(symbols.size() == contexts.size());
    TGztLemmas res;
    if (!symbols.empty()) {
        res = symbols[0]->GetAllGazetteerForms(contexts[0]);
        TGztLemmas next;
        for (size_t pos = 1; pos < symbols.size(); ++pos) {
            TGztLemmas symbolForms = symbols[pos]->GetAllGazetteerForms(contexts[pos]);
            const bool space = symbols[pos]->GetProperties().Test(PROP_SPACE_BEFORE);
            next.clear();
            next.reserve(res.size() * symbolForms.size());
            for (TGztLemmas::const_iterator iPrefix = res.begin(); iPrefix != res.end(); ++iPrefix) {
                for (TGztLemmas::const_iterator iSuffix = symbolForms.begin(); iSuffix != symbolForms.end(); ++iSuffix) {
                    next.push_back(*iPrefix);
                    if (space)
                        next.back() += wchar16(' ');
                    next.back() += *iSuffix;
                }
            }
            DoSwap(res, next);
        }
    }
    return res;
}

template <class TSymbolPtr>
inline void ExpungeChildrenGztArticles(const TVector<TSymbolPtr>& symbols, TVector<TDynBitMap>& contexts) {
        Y_ASSERT(symbols.size() == contexts.size());
        for (size_t i = 0; i < symbols.size(); ++i) {
            symbols[i]->ExpungeChildrenGztArticles(contexts[i]);
        }
}

namespace NPrivate {

class TNominativeFormBuilder {
private:
    class TImpl;
    THolder<TImpl> Impl;
public:
    TNominativeFormBuilder();
    ~TNominativeFormBuilder();
    void AddSymbol(TInputSymbol& s, const TDynBitMap& ctx);
    TUtf16String BuildForm();
};

template <class TSymbolPtr>
inline void FillNominativeFormBuilder(TNominativeFormBuilder& builder, const TVector<TSymbolPtr>& symbols, const TVector<TDynBitMap>& contexts) {
    Y_ASSERT(symbols.size() == contexts.size());
    TVector<TDynBitMap>::const_iterator iCtx = contexts.begin();
    for (typename TVector<TSymbolPtr>::const_iterator iSymb = symbols.begin(); iSymb != symbols.end(); ++iSymb, ++iCtx) {
        if (iSymb->Get()->GetChildren().empty()) {
            builder.AddSymbol(**iSymb, *iCtx);
        } else {
            FillNominativeFormBuilder(builder, iSymb->Get()->GetChildren(), iSymb->Get()->GetContexts());
        }
    }
}

} // NPrivate

template <class TSymbolPtr>
inline TUtf16String GetNominativeForm(const TVector<TSymbolPtr>& symbols, const TVector<TDynBitMap>& contexts) {
    NPrivate::TNominativeFormBuilder builder;
    FillNominativeFormBuilder(builder, symbols, contexts);
    return builder.BuildForm();
}

TUtf16String ToCamelCase(const TWtringBuf& str);

}  // NSymbol
