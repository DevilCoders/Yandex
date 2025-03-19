#pragma once

#include "combination_builder.h"

#include <kernel/remorph/facts/fact.h>
#include <kernel/remorph/common/article_util.h>

extern const TString CITY;

bool HasGeoAncestor(const NGzt::TArticlePtr& childArticle, const NGzt::TArticlePtr& parentArticle);

class TMineAddress;
typedef TIntrusivePtr<TMineAddress> TMineAddressPtr;

class TMineAddress : public TSimpleRefCount<TMineAddress> {
private:
    THashMultiMap<TStringBuf, TVector<NGzt::TArticlePtr> > Articles;
    THashMultiMap<TStringBuf, TUtf16String > FieldOriginalTexts;
    TUtf16String Numbers;
    TStringStream OriginalText;
    TVector<TString> AgreededFields;
    NFact::TFactPtr Fact;
    std::pair<size_t, size_t> OriginalPositions;

public:
    TMineAddress(const NFact::TFactPtr& addrFact, const std::pair<size_t, size_t>& sourcePositions);

    TString FirstVariantAsStroka() const;

    TString FirstVariantAsFullAnswer() const;

    TVector<TMineAddressPtr> CombineWithGeoPart(TMineAddressPtr geo) const;

    bool IsAgreedWithGeoPart(TMineAddressPtr geo) const;

    bool HasField(const TString& field) const {
        THashMultiMap<TStringBuf, TVector<NGzt::TArticlePtr> >::const_iterator iter = Articles.find(field);
        if (iter == Articles.end())
            return false;

        return iter->second.size() > 0;
    }

    bool HasCity() const {
        return HasField(CITY);
    }

    bool HasStreetNumber() const;

    //smaller means our address is subset of param address
    bool IsSmaller(TMineAddressPtr fact) const;

    size_t GetMaxFilledIndex() const;

    size_t GetMinFilledIndex() const;

    const std::pair<size_t, size_t>& GetOriginalPositions() const {
        return OriginalPositions;
    }

    TUtf16String GetField(const TString& field) const {
        return GetFirstFieldValue(field);
    }

private:
    bool GetSubField(const NFact::TFactPtr& geoFact, const TString& fieldName, const TString& subFieldName,
                     NFact::TFieldValuePtr* retField) const;

    TVector<NGzt::TArticlePtr> GetFieldArticles(const TString& field) const {
        THashMultiMap<TStringBuf, TVector<NGzt::TArticlePtr> >::const_iterator iter = Articles.find(field);
        if (iter == Articles.end())
            return TVector<NGzt::TArticlePtr>();

        return iter->second;
    }

    bool HasNumbers() const {
        return Numbers.length() > 0;
    }

    TUtf16String GetFieldOriginalText(const TString& field) const {
        THashMultiMap<TStringBuf, TUtf16String >::const_iterator iter = FieldOriginalTexts.find(field);
        if (iter == FieldOriginalTexts.end())
            return TUtf16String();

        return iter->second;
    }

    TUtf16String GetFirstFieldValue(const TString& field) const {
        TUtf16String ret = NGztSupport::GetLemma(GetFieldArticles(field)[0]);
        if (!!ret)
            return ret;

        return GetFieldOriginalText(field);
    }

    //smaller means our address is subset of param address (here all fields but numbers and descriptors)
    static bool IsSmaller(TVector<NGzt::TArticlePtr> self, TVector<NGzt::TArticlePtr> other);

    TMineAddress(const NFact::TFactPtr& addrFact, const std::pair<size_t, size_t>& sourcePositions, const TVector<NGzt::TArticlePtr>& combination,
                 const TVector<TString>& agreededFields, const TVector<NGzt::TArticlePtr>& descriptionActicles,
                 const TVector<TUtf16String>& combinationOriginalTexts, TVector<TUtf16String>& descriptionOriginalTexts);
};
