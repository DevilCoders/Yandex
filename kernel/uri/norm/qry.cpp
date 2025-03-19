/*
 *  Created on: Dec 3, 2011
 *      Author: albert@
 *
 * $Id$
 */

#include "qry.h"

namespace Nydx {
namespace NUriNorm {

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
    return **Iter;
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
    for (TIterColl<TpColl> it(coll); ; )
    {
        str += it->Key();
        if (it->Val().IsInited()) {
            str += '=';
            str += it->Val();
        }
        if (!++it)
            break;
        str += sep;
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

bool TQueryNormalizer::DoPropagate()
{
    Cleanup(false);
    if (!Parsed_)
        return false;
    TStringBuf val;
    if (!Cgi_.Empty()) {
        CgiStr_.remove(0);
        Cgi_.Print(CgiStr_, Delim_);
        val = CgiStr_;
    }
    return SetFieldImpl(val);
}

void TQueryNormalizer::CleanupImpl(TStringBuf qry)
{
    Cgi_.Clear();
    if (qry.empty())
        return;

    if (Lower_) {
        for (size_t idx = 0; idx != qry.length(); ++idx)
        {
            if (isupper(qry[idx])) {
                QryStr_.clear();
                QryStr_.reserve(qry.length());
                QryStr_.AppendNoAlias(qry.data(), idx);
                do
                    QryStr_ += tolower(qry[idx]);
                while (++idx != qry.length());
                qry = QryStr_;
                break;
            }
        }
    }

    if (qry.empty())
        return;

    char sep = '&';
    TStringBuf val = qry.NextTok(sep);
    // the delimiter not found
    if (!qry.IsInited() && Flags_.GetQryAltSemicol()) {
        // use ';' as separator if no '&' have been found
        sep = ';';
        qry = val.SplitOff(sep);
    }

    if (qry.IsInited() && !Flags_.GetQryNormDelim())
        Delim_ = sep;

    while (true)
    {
        if (val.IsInited() && (!val.empty() || !Flags_.GetQryRemoveEmpty()))
            Cgi_.Add(val);
        if (!qry.IsInited())
            break;
        val = qry.NextTok(sep);
    }
}

}
}
