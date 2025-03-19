/*
 *  Created on: Dec 3, 2011
 *      Author: albert@
 *
 * $Id$
 */

#include <util/generic/map.h>
#include <util/generic/vector.h>

#include "host_amzn.h"

namespace Nydx {
namespace NUriNorm {
namespace NAmzn {

/**
 * Normalizes the Amazon path.
 */
class THostSpec::TImpl
{
public:
    enum Type {
        PP_DP,
        PP_GP,
        PP_O,
        PP_B,
        PP_E,
        PP_EXEC,
        PP_PRS, // reviews
        PP_PR1, // review

        // add before this line
        PP_NONE
    };

protected:
    typedef std::pair<Type, TStringBuf> TdEntry;
    typedef TVector<TdEntry> TdEntries;
    typedef std::pair<Type, TdEntries> TdVal;
    typedef TMap<TString, TdVal> TdMap;
    typedef TVector<TStringBuf> TdStrList;
    struct TTypeEntry
    {
        TStringBuf Pref;
        TdStrList List;
        bool KeepQry;
        TStringBuf PrefOrig;
        TTypeEntry()
            : KeepQry(true)
        {}
    };

protected:
    TdMap Map_;
    TTypeEntry TypeList_[PP_NONE];

protected:
    TdEntries &InitEntry(const TStringBuf &key, Type type)
    {
        TdMap::iterator it = Map_.insert(TdMap::value_type(TString(key), TdVal())).first;
        TypeList_[type].PrefOrig = it->first;
        it->second.first = type;
        return it->second.second;
    }

    bool Cleanup(TPartNormalizers &norm) const;

public:
    TImpl();
    bool Normalize(TPartNormalizers &norm) const;
};

THostSpec::TImpl::TImpl()
{
    // http://en.wikipedia.org/wiki/Amazon_Standard_Identification_Number
    InitEntry("dp", PP_DP);
    {
        TdEntries &val = InitEntry("gp", PP_GP);
        val.push_back(TdEntry(PP_DP, TStringBuf("product/")));
    }
    {
        TdEntries &val = InitEntry("o", PP_O);
        val.push_back(TdEntry(PP_DP, TStringBuf("ASIN/")));
    }
    InitEntry("b", PP_B);
    InitEntry("e", PP_E);
    {
        TdEntries &val = InitEntry("exec", PP_EXEC);
        val.push_back(TdEntry(PP_DP, TStringBuf("obidos/tg/detail/-/")));
    }
    InitEntry("product-reviews", PP_PRS);
    InitEntry("review", PP_PR1);

    {
        TTypeEntry &entry = TypeList_[PP_DP];
        // these might come after leading "/dp/"
        entry.List.push_back(TStringBuf("product-reviews/"));
        entry.List.push_back(TStringBuf("product-description/"));
        entry.KeepQry = false;
    }
    {
        TTypeEntry &entry = TypeList_[PP_PRS];
        entry.KeepQry = false;
    }
    {
        TTypeEntry &entry = TypeList_[PP_PR1];
        entry.KeepQry = false;
    }
}

bool THostSpec::TImpl::Normalize(TPartNormalizers &norm) const
{
    bool changed = false;

    const TStringBuf &path = norm.Path.GetBuf();
    const size_t pos = path.find_last_of('/'); // must exist
    if ((path.SubStr(pos + 1)).StartsWith(TStringBuf("ref="))) {
        changed = true;
        norm.Path.Trunc(0 == pos ? 1 : pos);
    }

    if (Cleanup(norm))
        changed = true;

    return changed;
}

bool THostSpec::TImpl::Cleanup(TPartNormalizers &norm) const
{
    TStringBuf path = norm.Path.GetBuf().SubStr(1); // skip the leading '/'
    TStringBuf ptail = path;
    TStringBuf ptype;
    bool needslash = true;

    // skip the irrelevant category or item name part
    // stop when recognized the type
    TdMap::const_iterator it;
    size_t i1 = 0;
    do {
        if (3 == ++i1)
            return false;
        ptype = ptail.NextTok('/'); // find next label
        if (!ptail.IsInited())
            return false;
        it = Map_.find(ptype);
    }
    while (Map_.end() == it);

    const TdVal &val = it->second;
    Type type = val.first;
    // look for possible sublabels, with a different type
    for (size_t i2 = 0; i2 != val.second.size(); ++i2)
    {
        const TdEntry &entry = val.second[i2];
        const TStringBuf &pref = entry.second;
        if (ptail.StartsWith(pref)) {
            // remove the label
            ptail.Skip(pref.length());
            // change the type
            if (type != entry.first) {
                type = entry.first;
                ptype.Clear();
            }
            break;
        }
    }

    // remove possible extraneous sublabels
    const TTypeEntry &entry = TypeList_[type];
    const TdStrList &preflist = entry.List;
    for (size_t j = 0; j != preflist.size(); ++j)
    {
        const TStringBuf &pref = preflist[j];
        if (ptail.StartsWith(pref)) {
            ptail.Skip(pref.length());
            break;
        }
    }

    if (!entry.KeepQry)
        norm.Query.ResetCGI();

    // figure out the top-level label again
    if (!entry.Pref.empty()) {
        ptype = entry.Pref;
        needslash = false;
    }
    else if (!entry.PrefOrig.empty())
        ptype = entry.PrefOrig;
    else if (ptype.empty())
         // don't know what to do
        return false;

    // check whether we will change anything
    if (needslash) {
        if (path.length() == (1 + ptype.length() + ptail.length()))
            if (path.StartsWith(ptype) && '/' == path[ptype.length()])
                return false;
    }
    else {
        if (path.length() == (ptype.length() + ptail.length()))
            if (path.StartsWith(ptype))
                return false;
    }

    // regenerate the path
    TString pathstr;
    pathstr += '/';
    pathstr += ptype;
    if (needslash)
        pathstr += '/';
    pathstr += ptail;
    norm.Path.SetParsed(pathstr);

    return true;
}

THostSpec::THostSpec()
    : Impl_(new TImpl())
{}

THostSpec::~THostSpec()
{
    delete Impl_;
}

bool THostSpec::Normalize(TPartNormalizers &norm) const
{
    return Impl_->Normalize(norm);
}


}
}
}
