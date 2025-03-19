#pragma once

/*
 *  Created on: Dec 3, 2011
 *      Author: albert@
 *
 * $Id$
 */

#include <util/generic/list.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

#include <library/cpp/uri/uri.h>

namespace Nydx {
namespace NUri {

class TQueryParam
{
    TString Str_;
    TStringBuf Key_;
    TStringBuf Val_;
    int Pos_; // 0 for "key", 1 for "key=[val]", 2 to get next key
    char Sep_;
    bool PathParam_;
private:
    bool static IsDelim(char c)
    {
        return c == ';' || c == '&';
    }
    void CheckSeparator()
    {
        if (Key_.size() > 0 && IsDelim(Key_[0])) {
            Sep_ = Key_[0];
            Key_.Skip(1);
        }
    }
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
        , Sep_('&')
        , PathParam_(false)
    {
        CheckSeparator();
    }
    TQueryParam(const TStringBuf &str)
        : Str_(TString{str})
        , Key_(Str_)
        , Val_(Key_.SplitOff('='))
        , Pos_(Val_.IsInited() ? 1 : 0)
        , Sep_('&')
        , PathParam_(false)
    {
        CheckSeparator();
    }
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
    bool operator==(const TQueryParam &other) const
    {
        return 0 == Compare(other);
    }
    bool operator!=(const TQueryParam &other) const
    {
        return 0 == Compare(other);
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
    TStringBuf Str() const
    {
        return (!Str_.empty() && IsDelim(Str_[0])) ?
                    TStringBuf(Str_).Skip(1) : Str_;
    }
    char Sep() const
    {
        return Sep_;
    }
    size_t TotLen() const
    {
        return Key().length() + Pos_ + Val().length(); // key=val
    }
    size_t MaxLen() const
    {
        return Max(Key().length(), Val().length());
    }
    bool IsPathParam() const {
        return PathParam_;
    }
    operator TStringBuf() {
        return Str();
    }

public: // to set values
    void SetPathParam() {
        PathParam_ = true;
    }
    TQueryParam SetValue(const TStringBuf &value);
};

class TQueryParams
{
public:
    typedef TList<TQueryParam> TdList;
    typedef TdList::iterator TdListIter;
    typedef TdList::const_iterator TdListCIter;

public:
    typedef TMultiMap<TQueryParam, TdListIter> TdColl;
    typedef TdColl::iterator TdCollIter;
    typedef TdColl::const_iterator TdCollCIter;

public:
    class TIter
    {
        friend class TQueryParams;
    private:
        TdCollIter It_;
    public:
        TIter()
        {}
        TIter(const TdCollIter &it)
            : It_(it)
        {}
        const TQueryParam &operator*() const
        {
            return *It_->second;
        }
        const TQueryParam *operator->() const
        {
            return &*It_->second;
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
        bool operator==(const TIter &other) const
        {
            return It_ == other.It_;
        }
        bool operator!=(const TIter &other) const
        {
            return !(*this == other);
        }
        operator TdCollIter &()
        {
            return It_;
        }
        operator const TdCollIter &() const
        {
            return It_;
        }
    };

private:
    TdColl Coll_;
    TdList List_;

public:
    const bool NoSort;
    const bool KeepDups;
    const bool ParsePath;

private:
    static TStringBuf Split(TStringBuf& str, char sep) {
        if (!str.IsInited())
            return TStringBuf();
        size_t sepPos = str.find(sep, str[0] == sep);
        if (sepPos == TStringBuf::npos) {
            TStringBuf res = str;
            str.Clear();
            return res;
        }
        TStringBuf res = str.SubStr(0, sepPos);
        str.Skip(sepPos);
        return res;
    }

public:
    TQueryParams(bool parsePath = true, bool nosort = true, bool keepdups = true)
        : NoSort(nosort)
        , KeepDups(keepdups)
        , ParsePath(parsePath)
    {}
    void Parse(TStringBuf str)
    {
        ::NUri::TUri uri;
        Y_ENSURE(uri.Parse(str, ::NUri::TFeature::FeaturesRecommended) == ::NUri::TUri::ParsedOK,
               "Invalid url in TQueryParams::Parse " << str);
        Parse(uri);
    }
    void Parse(const ::NUri::TUri& uri);
    template <typename T> void Add(const T &val)
    {
        TdList::value_type entry(val);
        if (!KeepDups && Coll_.count(entry) > 0) {
            return;
        }
        List_.push_back(entry);
        Coll_.insert(TdColl::value_type(entry, --List_.end()));
    }
    void Clear()
    {
        Coll_.clear();
        List_.clear();
    }
    bool Empty() const
    {
        return List_.empty();
    }
    size_t Size() const
    {
        return List_.size();
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
    bool Has(const TStringBuf &key)
    {
        TIter itb;
        TIter ite;
        Get(key, itb, ite);
        return itb != ite;
    }
    bool Has(const TQueryParam &param)
    {
        return Coll_.find(param) != Coll_.end();
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
        List_.erase(it->second);
        Coll_.erase(it);
    }
    void Erase(const TdCollIter &itb, const TdCollIter &ite)
    {
        for (TdCollIter it = itb; it != ite; ++it)
            List_.erase(it->second);
        Coll_.erase(itb, ite);
    }
    bool SetValue(const TStringBuf &key, const TStringBuf &newValue);
    void Print(TString &str, char sep = '&') const
    {
        if (!Empty())
            if (NoSort)
                PrintList(str, sep);
            else
                PrintSort(str, sep);
    }
    void Save(::NUri::TUri& uri) const;

private:
    void ParseImpl(TStringBuf str, char sep, bool path = false);

protected:
    void PrintSort(TString &str, char sep) const;
    void PrintList(TString &str, char sep) const;
};

}
}

using Nydx::NUri::TQueryParams;
