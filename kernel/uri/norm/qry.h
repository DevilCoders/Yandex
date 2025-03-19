#pragma once

/*
 *  Created on: Dec 3, 2011
 *      Author: albert@
 *
 * $Id$
 */


#include "base.h"
#include "flags.h"

#include <util/generic/list.h>
#include <util/generic/vector.h>
#include <util/generic/map.h>

namespace Nydx {
namespace NUriNorm {

class TQueryParam
{
    TString Str_;
    TStringBuf Key_;
    TStringBuf Val_;
    int Pos_; // 0 for "key", 1 for "key=[val]", 2 to get next key
public:
    // this constructor for lower/upper bound key-based lookups
    TQueryParam(const TStringBuf &key, bool upb)
        : Key_(key)
        , Pos_(upb ? 2 : 0)
    {}
    // this constructor for adding entries
    TQueryParam(const TString &str)
        : Str_(str)
        , Key_(Str_)
        , Val_(Key_.SplitOff('='))
        , Pos_(Val_.IsInited() ? 1 : 0)
    {}
    TQueryParam(const TStringBuf &str)
        : Key_(str)
        , Val_(Key_.SplitOff('='))
        , Pos_(Val_.IsInited() ? 1 : 0)
    {}
public: // for TSet
    int Compare(const TQueryParam &other) const
    {
        int ret = Key().compare(other.Key());
        if (0 != ret)
            return ret;
        ret = Pos_ - other.Pos_;
        if (0 != ret)
            return ret;
        return Val().compare(other.Val());
    }
    bool operator<(const TQueryParam &other) const
    {
        return 0 > Compare(other);
    }
public: // to get values
    const TStringBuf &Key() const
    {
        return Key_;
    }
    const TStringBuf &Val() const
    {
        return Val_;
    }
    size_t TotLen() const
    {
        return Key().length() + Pos_ + Val().length(); // key=val
    }
    size_t MaxLen() const
    {
        return Max(Key().length(), Val().length());
    }
};

class TQueryParams
{
public:
    typedef TList<const TQueryParam *> TdList;
    typedef TdList::iterator TdListIter;
    typedef TdList::const_iterator TdListCIter;
    typedef TVector<TdListIter> TdListIters;

public:
    typedef TMap<TQueryParam, TdListIters> TdColl;
    typedef TdColl::iterator TdCollIter;
    typedef TdColl::const_iterator TdCollCIter;

public:
    class TIter
    {
        TdCollIter It_;
    public:
        TIter() = default;
        TIter(const TdCollIter &it)
            : It_(it)
        {}
        const TQueryParam &operator*() const
        {
            return It_->first;
        }
        const TQueryParam *operator->() const
        {
            return &It_->first;
        }
        TIter &operator++()
        {
            ++It_;
            return *this;
        }
        TIter &operator--()
        {
            --It_;
            return *this;
        }
        operator TdCollIter &()
        {
            return It_;
        }
        operator const TdCollIter &() const
        {
            return It_;
        }
        bool operator==(const TIter &other) const
        {
            return It_ == other.It_;
        }
        bool operator!=(const TIter &other) const
        {
            return !(*this == other);
        }
    };

private:
    TdColl Coll_;
    TdList List_;

public:
    const bool NoSort;
    const bool KeepDups;

public:
    TQueryParams(bool nosort = false, bool keepdups = false)
        : NoSort(nosort)
        , KeepDups(keepdups)
    {}
    void Parse(TStringBuf str, char sep = '&')
    {
        while (str.IsInited())
            Add(str.NextTok(sep));
    }
    template <typename T> const TQueryParam &Add(const T &val)
    {
        const TdColl::value_type entry(val, TdListIters());
        std::pair<TdCollIter, bool> res = Coll_.insert(entry);
        const TQueryParam &obj = res.first->first;
        if (NoSort && (res.second || KeepDups))
            res.first->second.push_back(List_.insert(List_.end(), &obj));
        return obj;
    }
    const TQueryParam *TryAdd(const TStringBuf &val)
    {
        return val.IsInited() ? &Add(val) : nullptr;
    }
    const TQueryParam *TryAdd(const TString &val)
    {
        return val.empty() ? nullptr : &Add(val);
    }
    void Clear()
    {
        Coll_.clear();
    }
    bool Empty() const
    {
        return Coll_.empty();
    }
    size_t Size() const
    {
        return Coll_.size();
    }
    TIter Begin()
    {
        return Coll_.begin();
    }
    TIter End()
    {
        return Coll_.end();
    }
    void Get(const TStringBuf &key, TIter &itb, TIter &ite)
    {
        itb = Coll_.lower_bound(TQueryParam(key, false));
        ite = Coll_.lower_bound(TQueryParam(key, true));
    }
    void GetEnd(const TIter &itb, TIter &ite)
    {
        ite = Coll_.lower_bound(TQueryParam(itb->Key(), true));
    }
    void EraseAll(const TStringBuf &key)
    {
        TIter itb, ite;
        Get(key, itb, ite);
        Erase(itb, ite);
    }
    void Erase(const TdCollIter &it)
    {
        if (NoSort)
            EraseImpl(it->second);
        Coll_.erase(it);
    }
    void Erase(const TdCollIter &itb, const TdCollIter &ite)
    {
        if (NoSort)
            for (TdCollIter it = itb; it != ite; ++it)
                EraseImpl(it->second);
        Coll_.erase(itb, ite);
    }
    void Print(TString &str, char sep = '&') const
    {
        if (!Empty())
            if (NoSort)
                PrintList(str, sep);
            else
                PrintSort(str, sep);
    }

private:
    void EraseImpl(const TdListIters &list)
    {
        for (size_t i = 0; i != list.size(); ++i)
            List_.erase(list[i]);
    }

protected:
    void PrintSort(TString &str, char sep) const;
    void PrintList(TString &str, char sep) const;
};

class TQueryNormalizer
    : public TNormalizerBase<::NUri::TUri::FieldQuery, TQueryNormalizer>
{
    const TFlags &Flags_;
    TQueryParams Cgi_;
    bool Parsed_; ///< true if the query was parsed
    TString CgiStr_;
    TString QryStr_;
    char Delim_;
    bool Lower_;
    bool Force_;

public:
    TQueryNormalizer(::NUri::TUri &uri, const TFlags &flags)
        : TdBase(uri, flags.GetNormalizeQry())
        , Flags_(flags)
        , Cgi_(!flags.GetQrySort(), !flags.GetQryRemoveDups())
        , Parsed_(false)
        , Delim_('&')
        , Lower_(flags.GetLowercaseQry() && !flags.GetLowercaseURL())
        , Force_(Lower_
            || !Cgi_.NoSort
            || !Cgi_.KeepDups
            || flags.GetQryRemoveEmpty()
            || flags.GetFlagMask(TFlags::ESCAPE_FRAGMENT_GROUP_MASK)
            || (flags.GetQryAltSemicol() && flags.GetQryNormDelim())
            )
    {}
    TQueryParams &GetCGI()
    {
        Cleanup(true);
        return Cgi_;
    }
    void Set(const TStringBuf &qry)
    {
        Parsed_ = true;
        CleanupImpl(qry);
    }
    void ResetCGI()
    {
        Cgi_.Clear();
        Parsed_ = true;
    }
    bool DoPropagate();

private:
    void Cleanup(bool force)
    {
        if (!Parsed_)
            if (force || Force_)
                Set(GetField());
    }

private:
    /**
     * Normalizes the query part of a URI.
     * - If no '&' separators are located, presume semicolons (';') are used for
     *   separation and replace each with an ampersand.
     * - Parse the string with @c TCgiParameters (which applies URI unescaping
     *   and sorts the keys), then sort the values for the same key.
     * @param[in] qry the query string (non-empty)
     */
    void CleanupImpl(TStringBuf qry);
};

}
}
