/*
 *  Created on: Dec 3, 2011
 *      Author: albert@
 *
 * $Id$
 */

#include "query_params.h"

namespace Nydx {
namespace NUri {

template <typename TpColl> struct TIterColl
{
    const TpColl &Coll;
    typename TpColl::const_iterator Iter;
    TIterColl(const TpColl &coll)
        : Coll(coll)
        , Iter(Coll.begin())
    {}
    inline const TQueryParam &operator*() const;
    const TQueryParam *operator->() const
    {
        return &(**this);
    }
    TIterColl &operator++()
    {
        ++Iter;
        return *this;
    }
    operator bool() const
    {
        return Coll.end() != Iter;
    }
};

template<> const TQueryParam &TIterColl<TQueryParams::TdList>::operator *() const
{
    return *Iter;
}

template<> const TQueryParam &TIterColl<TQueryParams::TdColl>::operator *() const
{
    return Iter->first;
}


template <typename TpColl> static
    void PrintQueryParams(const TpColl &coll, TString &str, char sep)
{
    if (coll.empty())
        return;

    size_t totlen = 0;
    for (TIterColl<TpColl> it(coll); it; ++it)
        totlen += 1 + it->TotLen(); // &key=val

    str.reserve(totlen - 1 + str.length()); // one too many '&'
    TIterColl<TpColl> it(coll);
    Concatenate(it, str, sep);
}

template <typename TpCollIter> static
    void Concatenate(TpCollIter &it, TString &str, char sep)
{
    if (!it)
        return;
    bool useCustomSeparator = sep != 0;
    while(true)
    {
        str += it->Key();
        if (it->Val().IsInited()) {
            str += '=';
            str += it->Val();
        }
        if (!++it)
            break;
        str += useCustomSeparator ? sep : it->Sep();
    }
}

static bool SetField(::NUri::TUriUpdate& uri, ::NUri::TField::EField field, const TString& value) {
    if (value.empty()) {
        return uri.Set(field, TStringBuf());
    }
    return uri.Set(field, value);
}

void TQueryParams::Parse(const ::NUri::TUri& uri)
{
    if (ParsePath) {
        TStringBuf path = uri.GetField(::NUri::TUri::TField::FieldPath);
        if (path.find(';') != TStringBuf::npos) {
            Split(path, ';');
            ParseImpl(path, ';', true);
        }
    }

    TStringBuf qry = uri.GetField(::NUri::TUri::TField::FieldQuery);
    char sep = '&';
    if (qry.find('&') == TStringBuf::npos) {
        sep = ';';
    }
    ParseImpl(qry, sep);
}

bool TQueryParams::SetValue(const TStringBuf& key, const TStringBuf& newValue) {
    TIter itb, ite;
    Get(key, itb, ite);
    if (itb == ite) {
        TString newParam(key);
        newParam.append('=');
        newParam.append(newValue);
        Add(newParam);
        return true;
    }

    TdCollIter cIt = itb;
    TdListIter lIt = cIt->second;
    *lIt = lIt->SetValue(newValue);

    Coll_.erase(cIt);
    Coll_.insert(TdColl::value_type(*lIt, lIt));
    return false;
}

void TQueryParams::Save(::NUri::TUri& uri) const
{
    Y_ENSURE(NoSort, "TQueryParams::Save only avaliable with NoSort");
    TString newQuery;
    ::NUri::TUriUpdate update(uri);
    TIterColl<TQueryParams::TdList> it(List_);
    if (ParsePath) {  // Reset path
        TStringBuf path = uri.GetField(::NUri::TField::FieldPath);

        TString newPath = TString{path.NextTok(';')};
        for (; it && it->IsPathParam(); ++it) {
            newPath += it->Sep();
            newPath += it->Key();
            if (it->Val().IsInited()) {
                newPath += '=';
                newPath += it->Val();
            }
        }
        SetField(update, ::NUri::TField::FieldPath, newPath);
        uri.Rewrite();
    }
    Concatenate(it, newQuery, 0);
    SetField(update, ::NUri::TField::FieldQuery, newQuery);
}

void TQueryParams::ParseImpl(TStringBuf str, char sep, bool path)
{
    while (str.IsInited()) {
        if (path) {
            TQueryParam param(Split(str, sep));
            param.SetPathParam();
            Add(param);
        } else {
            Add(Split(str, sep));
        }
    }
}

void TQueryParams::PrintSort(TString &str, char sep) const
{
    PrintQueryParams(Coll_, str, sep);
}

void TQueryParams::PrintList(TString &str, char sep) const
{
    PrintQueryParams(List_, str, sep);
}

TQueryParam TQueryParam::SetValue(const TStringBuf& value) {
    TString str = Str_.substr(0, Str_.size() - Val_.size()); // Copy all but value
    if (Pos_ == 0 && !value.empty())
        str.append('=');
    if (Pos_ == 1 && value.empty())
        str.pop_back();
    str.append(value);
    TQueryParam result = TQueryParam(str);
    if (IsPathParam())
        result.SetPathParam();
    return result;
}

}
}
