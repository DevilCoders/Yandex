#pragma once

#include "solver.h"
#include "thingy.h"

#include <tools/clustermaster/communism/core/core.h>

#include <util/datetime/base.h>
#include <util/generic/intrlist.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/string/printf.h>
#include <util/system/spinlock.h>

using namespace NCommunism;

struct TBatch;

template <>
struct TLess< std::pair<unsigned, unsigned> > {
    inline bool operator()(const std::pair<unsigned, unsigned>& first, const std::pair<unsigned, unsigned>& second) const noexcept {
        if (first.first == second.first) {
            return first.second < second.second;
        } else {
            return first.first < second.first;
        }
    }
};

struct TClaims: protected TMap<unsigned, unsigned, TLess<unsigned>, TAllocator> {
    typedef TMap<unsigned, unsigned, TLess<unsigned>, TAllocator> TBase;
    typedef std::pair<unsigned, unsigned> TSharedPartKey;
    typedef TMap<TSharedPartKey, unsigned, TLess<TSharedPartKey>, TAllocator> TSharedParts;
    typedef value_type::first_type TKey;
    typedef value_type::second_type TVal;
    typedef TBase::const_iterator TConstIterator;

    TSharedParts SharedParts;

    const_iterator Begin() const noexcept {
        return begin();
    }

    const_iterator End() const noexcept {
        return end();
    }

    bool Combine(const TClaims& right);

    bool Empty() const {
        return empty() && SharedParts.empty();
    }

    void Clear() {
        clear();
        SharedParts.clear();
    }

    void Swap(TClaims& right) {
        swap(right);
        SharedParts.swap(right.SharedParts);
    }

    double ToNormDouble(TVal value) const {
        return static_cast<double>(value) / static_cast<double>(Max<TVal>());
    }

    TString ToStringMaybeWithRound(double value, bool round) const {
        return (round ? Sprintf("%0.6g", value) : ToString<double>(value));
    }

    void OutRes(IOutputStream& out, bool round) const {
        out << '{';
        for (const_iterator i = begin(); i != end(); ++i) {
            if (i != begin()) {
                out << ',';
            }
            TMaybe<TString> key = NGlobal::KeyMapper.MappedValue(i->first);
            Y_VERIFY(key.Defined(), "not mapped key");
            double value = ToNormDouble(i->second);
            out << *key << ':' << ToStringMaybeWithRound(value, round);
        }
        out << '}';
    }

    void OutShared(IOutputStream& out, bool round) const {
        out << '{';
        for (TSharedParts::const_iterator i = SharedParts.begin(); i != SharedParts.end(); ++i) {
            if (i != SharedParts.begin()) {
                out << ',';
            }
            TMaybe<TString> name = NGlobal::KeyMapper.MappedValue(i->first.first);
            TMaybe<TString> key = NGlobal::KeyMapper.MappedValue(i->first.second);
            Y_VERIFY(name.Defined() && key.Defined(), "not mapped name or key");
            double value = ToNormDouble(i->second);
            out << *key << ":{value:" << ToStringMaybeWithRound(value, round) << ",name:" << *name << '}';
        }
        out << '}';
    }

    void OutResMultiline(IOutputStream& out) const {
        for (const_iterator i = begin(); i != end(); ++i) {
            TMaybe<TString> key = NGlobal::KeyMapper.MappedValue(i->first);
            Y_VERIFY(key.Defined(), "not mapped key");
            double value = ToNormDouble(i->second);
            out << *key << ": " << ToStringMaybeWithRound(value, true) << '\n';
        }
    }

    void Out(IOutputStream& out) const {
PROFILE_ACQUIRE(PROFILE_CLAIMS_OUTPUT)
        out << '{';
        out << "res:";
        OutRes(out, false);
        out << ",shared:";
        OutShared(out, false);
        out << '}';
    }
};

template <>
inline void Out<TClaims>(IOutputStream& out, const TClaims& claims) {
    claims.Out(out);
}

struct TRequest: TClaims, TCopyableIntrusiveListItem<TRequest> {
    TBatch* const Batch;
    const NCore::TKey Key;

    enum EState {
        DEFINED,
        REQUESTED,
        GRANTED
    };

    EState State;

    NCore::TActionId GrantActionId;

    size_t IndexNumber;

    TInstant DisclaimForecast;

    unsigned Priority;
    TDuration Duration;
    TString Label;

    struct TRejectException: yexception {};

    struct TExtractKey {
        NCore::TKey operator()(const TRequest& from) const noexcept {
            return from.Key;
        }
    };

    TRequest(TBatch* batch, NCore::TKey key)
        : Batch(batch)
        , Key(key)
        , State(DEFINED)
        , GrantActionId(0)
        , IndexNumber(0)
        , Priority(0)
        , Duration(TDuration::Max())
    {
    }

    TRequest(TBatch* batch, const NCore::TDefineMessage& message, const TRequest& right)
        : TClaims(right)
        , Batch(batch)
        , Key(message.GetKey())
        , State(DEFINED)
        , GrantActionId(0)
        , IndexNumber(0)
        , Priority(0)
        , Duration(TDuration::Max())
    {
    }

    TRequest(TBatch* batch, const NCore::TDefineMessage& message);
};

typedef TIntrusiveList<TRequest> TRequests;


struct TRequestsHandleImpl: TAtomicRefCount<TRequestsHandleImpl>, TNonCopyable {
public:
    TRequestsHandleImpl()
        : Requested(new TRequests())
        , Granted(new TRequests())
        , Lock(new TSpinLock())
    {
    }

    THolder<TRequests> Requested;
    THolder<TRequests> Granted;

    THolder<TSpinLock> Lock;
};

class TRequestsHandle {
public:
    TRequestsHandle()
        : Impl(new TRequestsHandleImpl())
    {
    }

    TIntrusivePtr<TRequestsHandleImpl> Impl;

    TRequestsHandleImpl* operator->() const {
        return Impl.operator->();
    }
};

struct TStateForecast: private TMap<TRequest::TKey, TInstant, TLess<TRequest::TKey>, TAllocator> {
    void Update(const TClaims& claims, TInstant deadline) {
        for (TClaims::TConstIterator i = claims.Begin(); i != claims.End(); ++i) {
            const iterator it = insert(value_type(i->first, TInstant::Max())).first;
            it->second = Min(it->second, deadline);
        }
    }

    TInstant Deduce(const TClaims& claims) const {
        TInstant ret = TInstant::Max();

        for (TClaims::TConstIterator i = claims.Begin(); i != claims.End(); ++i) {
            const const_iterator it = find(i->first);
            if (it != end()) {
                ret = Min(ret, it->second);
            }
        }

        return ret;
    }
};
