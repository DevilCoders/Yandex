/*
 *  Created on: Dec 3, 2011
 *      Author: albert@
 *
 * $Id$
 */

#include "coll.h"
#include "host_goog.h"

#include <library/cpp/string_utils/quote/quote.h>

namespace Nydx {
namespace NUriNorm {
namespace NGoog {

struct TIncludeSet
    : public TStrBufSet
{};

struct TExcludeSet
    : public TStrBufSet
{};

struct TCgiArgsFind
    : public TIncludeSet
{
    TCgiArgsFind()
    {
        Add("hl");
        Add("prmd");
        Add("q");
        Add("sa");
        Add("source");
        Add("tbm");
        Add("tbo");
    }
};

struct TCgiArgsMaps
    : public TExcludeSet
{
    TCgiArgsMaps()
    {
        Add("ei");
        Add("ved");
    }
};

struct TCgiArgsBook
    : public TExcludeSet
{
    TCgiArgsBook()
    {
        Add("ct");
        Add("dq");
        Add("ei");
        Add("f");
        Add("lpg");
        Add("num");
        Add("oi");
        Add("ots");
        Add("q");
        Add("resnum");
        Add("sa");
        Add("sig");
        Add("source");
        Add("ved");
    }
};

struct TCgiArgsHelp
    : public TIncludeSet
{
    TCgiArgsHelp()
    {
        Add("answer");
        Add("hl");
        Add("topic");
    }
};

struct TCgiArgsHome
    : public TExcludeSet
{
    TCgiArgsHome()
    {
        Add("gws_rd");
        Add("gfe_rd");
    }
};

enum EType
{
    ETypeDef,

    ETypeHome,
    ETypeFind,
    ETypeRedir,
    ETypeMaps,
    ETypeIG,
    ETypeMail,
    ETypeBooks,
    ETypeImg,
    ETypeProd,
    ETypeNews,
    ETypeHelp,

    ETypeMAX
};

struct THostPathTypeMap
    : public TMapT<TStringBuf, std::pair<EType, TMapT<TStringBuf, EType> > >
{
    THostPathTypeMap();
    EType GetType(const TStringBuf &hpref, TStringBuf path) const;
};

THostPathTypeMap::THostPathTypeMap()
{
    {
        TdVal &val = Val("");
        val.first = ETypeMAX;
        TdVal::second_type &map = val.second;
        map.Val("") = ETypeHome;
        map.Val("webhp") = ETypeHome;
        map.Val("search") = ETypeFind;
        map.Val("ig") = ETypeIG;
        map.Val("url") = ETypeRedir;
        map.Val("mail") = ETypeMail;
        map.Val("imgres") = ETypeImg;
        map.Val("imghp") = ETypeImg;
        map.Val("products") = ETypeProd;
        // copy to other prefix
        Val("www") = val;
    }
    {
        TdVal &val = Val("books");
        val.first = ETypeBooks;
        val.second.Val("support") = ETypeHelp;
    }
    {
        TdVal &val = Val("maps");
        val.first = ETypeMaps;
    }
    {
        TdVal &val = Val("mail");
        val.first = ETypeMail;
    }
    {
        TdVal &val = Val("news");
        val.first = ETypeNews;
    }
    {
        TdVal &val = Val("url");
        val.first = ETypeRedir;
    }
}

EType THostPathTypeMap::GetType(const TStringBuf &hpref, TStringBuf path) const
{
    const TdVal *pht = Get(hpref);
    if (nullptr == pht)
        return ETypeMAX;

    if (!path.empty() && '/' == path[0])
        path.Skip(1);
    const TStringBuf label = path.NextTok('/');
    const EType *ppt = pht->second.Get(label);

    return nullptr != ppt ? *ppt : pht->first;
}


static const TCgiArgsFind CgiArgsFind;
static const TCgiArgsMaps CgiArgsMaps;
static const TCgiArgsBook CgiArgsBook;
static const TCgiArgsHelp CgiArgsHelp;
static const TCgiArgsHome CgiArgsHome;
static const THostPathTypeMap TypeMap;

/**
 * Normalizes the Google path.
 */
class THostSpecImpl
{
public:
    static bool Normalize(const TStringBuf &hpref, TPartNormalizers &norm);

private:
    static bool Normalize(EType type, TPartNormalizers &norm);
    static bool NormalizeHome(TPartNormalizers &norm);
    static bool NormalizeRedir(TPartNormalizers &norm);

private:
    template <typename TpSet>
    static bool Normalize(TPartNormalizers &norm, const TpSet &set)
    {
        return Normalize(norm.Query.GetCGI(), set);
    }
    static bool Normalize(TQueryParams &cgi, const TIncludeSet &set);
    static bool Normalize(TQueryParams &cgi, const TExcludeSet &set);
};

bool THostSpecImpl::Normalize(const TStringBuf &hpref, TPartNormalizers &norm)
{
    bool changed = false;

    norm.Scheme.MakeHttp();

    if (!norm.Query.Disabled()) {
        const EType type = TypeMap.GetType(hpref, norm.Path.GetBuf());
        if (Normalize(type, norm))
            changed = true;
    }

    return changed;
}

bool THostSpecImpl::Normalize(EType type, TPartNormalizers &norm)
{
    switch (type)
    {
    case ETypeFind:
    case ETypeProd:
    case ETypeNews:
        return Normalize(norm, CgiArgsFind);

    case ETypeMaps:
        return Normalize(norm, CgiArgsMaps);

    case ETypeHelp:
        return Normalize(norm, CgiArgsHelp);

    case ETypeBooks:
        return Normalize(norm, CgiArgsBook);

    case ETypeRedir:
        return NormalizeRedir(norm);

    case ETypeHome:
        return NormalizeHome(norm);

    default:
        break;
    }

    return false;
}

bool THostSpecImpl::NormalizeHome(TPartNormalizers &norm)
{
    const ::NUri::TUri &uri = norm.GetURI();
    Normalize(norm, CgiArgsHome);
    if (!uri.IsNull(::NUri::TUri::FlagQuery))
        return false;
    if (uri.IsNull(::NUri::TUri::FlagFrag))
        return false;
    ::NUri::TUri &ruri = norm.GetMutableURI();
     // copy fragment to query; not entirely correct but close enough
    ruri.FldMemSet(::NUri::TUri::FieldPath, "/search");
    ruri.FldMemUse(::NUri::TUri::FieldQuery, uri.GetField(::NUri::TUri::FieldFrag));
    ruri.FldClr(::NUri::TUri::FieldFrag);
    return true;
}

bool THostSpecImpl::NormalizeRedir(TPartNormalizers &norm)
{
    TQueryParams::TIter itb, ite;
    norm.Query.GetCGI().Get("url", itb, ite);
    if (itb == ite)
        return false;
    TStringBuf urival = itb->Val();
    TTempBuf uribuf(urival.length());
    urival = CgiUnescapeBuf(uribuf.Data(), urival);
    ::NUri::TUri uri;
    ::NUri::TUri::EParsed res = norm.Flags().Parse(uri, urival);
    if (::NUri::TUri::ParsedOK != res)
        return false;
    if (!uri.IsValidGlobal()) {
        uri.Merge(norm.Query.GetURI());
        if (!uri.IsValidGlobal())
            return false;
    }
    norm.SetURI(uri);
    return true;
}

bool THostSpecImpl::Normalize(TQueryParams &cgi, const TIncludeSet &set)
{
    const size_t size = cgi.Size();
    for (TQueryParams::TIter it = cgi.Begin(), ite; it != cgi.End(); )
    {
        cgi.GetEnd(it, ite);
        if (it == ite)
            break;
        if (!set.Has(it->Key()))
            cgi.Erase(it, ite);
        it = ite;
    }
    return cgi.Size() != size;
}

bool THostSpecImpl::Normalize(TQueryParams &cgi, const TExcludeSet &set)
{
    const size_t size = cgi.Size();
    for (TExcludeSet::TdIter it = set.Begin(); it != set.End(); ++it)
        cgi.EraseAll(*it);
    return cgi.Size() != size;
}


bool THostSpec::Normalize(const TStringBuf &hpref, TPartNormalizers &norm) const
{
    return THostSpecImpl::Normalize(hpref, norm);
}

}
}
}
