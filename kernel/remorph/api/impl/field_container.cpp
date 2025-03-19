#include "field_container.h"

#include "fields.h"
#include "compound_fields.h"
#include "articles.h"

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/charset/wide.h>

namespace NRemorphAPI {

namespace NImpl {

TFieldContainerBase::TFieldContainerBase(const NFact::TFieldValueContainer& c, const NText::TWordSymbols& tokens)
    : Container(c)
    , Tokens(tokens)
{
}

IArticles* TFieldContainerBase::GetArticles() const {
    TVector<NGzt::TArticlePtr> articles = Container.GetAllArticles();
    return new TArticles(articles);
}

bool TFieldContainerBase::HasArticle(const char* name) const {
    return Container.HasArticle(UTF8ToWide(name));
}

const char* TFieldContainerBase::GetRule() const {
    return Container.GetRuleName().data();
}

double TFieldContainerBase::GetWeight() const {
    return Container.GetWeight();
}

// Order fields by positions
struct TFieldOrder {
    inline bool operator() (const NFact::TFieldValuePtr& l, const NFact::TFieldValuePtr& r) const {
        return l->GetSrcPos() < r->GetSrcPos();
    }
    inline bool operator() (const NFact::TCompoundFieldValuePtr& l, const NFact::TCompoundFieldValuePtr& r) const {
        return l->GetSrcPos() < r->GetSrcPos();
    }
};

IFields* TFieldContainerBase::GetFields() const {
    const THashMultiMap<TStringBuf, NFact::TFieldValuePtr>& values = Container.GetValues();
    TVector<NFact::TFieldValuePtr> fields;
    for (THashMultiMap<TStringBuf, NFact::TFieldValuePtr>::const_iterator i = values.begin(); i != values.end(); ++i) {
        fields.push_back(i->second);
    }
    ::StableSort(fields.begin(), fields.end(), TFieldOrder());
    return new TFields(this, fields, Tokens);
}

IFields* TFieldContainerBase::GetFields(const char* fieldName) const {
    TVector<NFact::TFieldValuePtr> fields = Container.GetValues(fieldName);
    ::StableSort(fields.begin(), fields.end(), TFieldOrder());
    return new TFields(this, fields, Tokens);
}

ICompoundFields* TFieldContainerBase::GetCompoundFields() const {
    const THashMultiMap<TStringBuf, NFact::TCompoundFieldValuePtr>& values = Container.GetCompoundValues();
    TVector<NFact::TCompoundFieldValuePtr> fields;
    for (THashMultiMap<TStringBuf, NFact::TCompoundFieldValuePtr>::const_iterator i = values.begin(); i != values.end(); ++i) {
        fields.push_back(i->second);
    }
    ::StableSort(fields.begin(), fields.end(), TFieldOrder());
    return new TCompoundFields(this, fields, Tokens);
}

ICompoundFields* TFieldContainerBase::GetCompoundFields(const char* fieldName) const {
    TVector<NFact::TCompoundFieldValuePtr> fields = Container.GetCompoundValues(fieldName);
    ::StableSort(fields.begin(), fields.end(), TFieldOrder());
    return new TCompoundFields(this, fields, Tokens);
}

} // NImpl

} // NRemorphAPI
