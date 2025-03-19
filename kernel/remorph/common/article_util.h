#pragma once

#include <kernel/gazetteer/gazetteer.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NGztSupport {

extern const TString MAIN_WORD;
extern const TString GZT_COUNTRY_TYPE;

TUtf16String GetLemma(const NGzt::TArticlePtr& article);

// The field can be complex name (dot-separated)
bool HasFieldValue(const NGzt::TMessage& msg, const TString& field, const TString& value);

inline TWtringBuf GetLanguageIndependentTitle(const TUtf16String& title) {
    return TWtringBuf(title).NextTok('/');
}

inline bool GetLanguageIndependentTitle(TUtf16String& title) {
    TWtringBuf t = GetLanguageIndependentTitle((const TUtf16String&)title);
    return t.size() != title.size() ? title.assign(t), true : false;
}

double GetGztWeight(const NGzt::TArticlePtr& a);

template <typename TResult>
inline bool GetMainGztWord(const NGzt::TArticlePtr& a, TResult& mainWord) {
    mainWord = 0;
    NGzt::TProtoFieldIterator<TResult> iter = a.IterField<TResult, const TString&>(MAIN_WORD);
    if (!iter.Ok())
        return false;
    mainWord = *iter;
    if (mainWord > 0) {
        // Value is 1-based. Decrement by one.
        --mainWord;
    }
    return !iter.Default();
}

bool HasGeoAncestor(const NGzt::TArticlePtr& childArticle, const THashSet<TUtf16String>& parentArticles);
bool HasGeoAncestor(const NGzt::TArticlePtr& childArticle, const THashSet<TUtf16String>& parentArticles, TUtf16String* newArticleTitle);

} // NGztSupport
