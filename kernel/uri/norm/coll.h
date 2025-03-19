#pragma once

/*
 *  Created on: Dec 3, 2011
 *      Author: albert@
 *
 * $Id$
 */


#include <util/generic/algorithm.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/strbuf.h>

namespace Nydx {
namespace NUriNorm {

template <typename TpKey, typename TpLess = TLess<TpKey> > class TSetT
{
public:
    typedef TSet<TpKey, TpLess> TdSet;
    typedef typename TdSet::const_iterator TdIter;

protected:
    TdSet Set_;

public:
    bool Has(const TpKey &key) const
    {
        return Set_.end() != Set_.find(key);
    }
    bool Add(const TpKey &key)
    {
        return Set_.insert(key).second;
    }
    TdIter Begin() const
    {
        return Set_.begin();
    }
    TdIter End() const
    {
        return Set_.end();
    }
};

template <typename TpKey, typename TpVal> class TMapT
{
public:
    typedef TpKey TdKey;
    typedef TpVal TdVal;
    typedef TMap<TpKey, TpVal> TdMap;
    typedef typename TdMap::const_iterator TdIter;

protected:
    TdMap Map_;

public:
    const TpVal *Get(const TpKey &key) const
    {
        const TdIter it = Map_.find(key);
        return Map_.end() != it ? &it->second : nullptr;
    }
    TdIter Begin() const
    {
        return Map_.begin();
    }
    TdIter End() const
    {
        return Map_.end();
    }
    template <typename TpArg>
    TpVal &Val(const TpArg &key)
    {
        return Map_[key];
    }
    template <typename TpArg1, typename TpArg2>
    TpVal &Val(const TpArg1 &arg1, const TpArg2 &arg2)
    {
        return Val(TpKey(arg1, arg2));
    }
    template <typename TpArg1, typename TpArg2, typename TpArg3>
    TpVal &Val(const TpArg1 &arg1, const TpArg2 &arg2, const TpArg3 &arg3)
    {
        return Val(TpKey(arg1, arg2, arg3));
    }
};

typedef TSetT<TStringBuf> TStrBufSet;

}
}
