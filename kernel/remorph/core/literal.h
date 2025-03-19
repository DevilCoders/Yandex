#pragma once

#include "types.h"
#include "util.h"

#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/generic/ylimits.h>
#include <util/generic/typetraits.h>
#include <util/stream/input.h>
#include <util/stream/output.h>

namespace NRemorph {

struct TLiteral {
    static const TLiteralId TYPE_MASK = ~(TStaticLimits<TLiteralId>::Max >> 3);
    static const TLiteralId ID_MASK = TStaticLimits<TLiteralId>::Max >> 3;
    static const size_t TYPE_SHIFT = sizeof(TLiteralId) * 8 - 3;

    static const TLiteralId INVALID = TStaticLimits<TLiteralId>::Max;
    static const TLiteralId MAX_ID = ID_MASK;

    // 3 bits
    enum EType {
        Ordinal = 0,
        Any     = 1,
        Bos     = 2,
        Eos     = 3,
        Marker  = 4,
        None    = 7,
    };

    TLiteralId Value;

    TLiteral()
        : Value(INVALID)
    {
    }
    TLiteral(TLiteralId id, EType type)
        : Value((type << TYPE_SHIFT) | (id & ID_MASK))
    {
        Y_ASSERT(0 == (id & TYPE_MASK));
    }

    inline EType GetType() const {
        return EType(Value >> TYPE_SHIFT);
    }

    inline TLiteralId GetId() const {
        return Value & ID_MASK;
    }

    inline bool IsNone() const {
        return GetType() == None;
    }
    inline bool IsAny() const {
        return GetType() == Any;
    }
    inline bool IsBOS() const {
        return GetType() == Bos;
    }
    inline bool IsEOS() const {
        return GetType() == Eos;
    }
    inline bool IsMarker() const {
        return GetType() == Marker;
    }
    inline bool IsAnchor() const {
        switch (GetType()) {
        case Bos:
        case Eos:
            return true;
        default:
            return false;
        }
    }
    inline bool IsZeroShift() const {
        switch (GetType()) {
        case Bos:
        case Eos:
        case Marker:
        case None:
            return true;
        default:
            return false;
        }
    }
    inline bool IsOrdinal() const {
        return GetType() == Ordinal;
    }
    inline bool operator==(TLiteral l) const {
        return Value == l.Value;
    }
};

template <class TLiteralTable>
inline TString ToString(const TLiteralTable& lt, TLiteral l) {
    return lt.ToString(l);
}

template <class TLiteralTable, class TSymbolIterator>
inline bool Compare(const TLiteralTable& lt, TLiteral l, TSymbolIterator& s) {
    switch (l.GetType()) {
    case TLiteral::Bos:
        return s.AtBegin();
    case TLiteral::Eos:
        return s.AtEnd();
    case TLiteral::Marker:
        return true;
    case TLiteral::None:
        return false;
    default:
        return !s.AtEnd() && (TLiteral::Any == l.GetType() || s.IsEqual(lt, l));
    }
}

template <class TImpl>
struct TLiteralTable: public TImpl {
    TString ToString(TLiteral l) const {
        switch (l.GetType()) {
        case TLiteral::None:
            return "NONE";
        case TLiteral::Any:
            return "ANY";
        case TLiteral::Bos:
            return "BOS";
        case TLiteral::Eos:
            return "EOS";
        case TLiteral::Marker:
            return "MARK";
        default:
            return TImpl::ToString(l);
        }
    }
    inline TLiteralId Size() const {
        return TImpl::Size();
    }
    inline TLiteralId GetPriority(TLiteral l) const {
        if (l.GetType() == TLiteral::Any) {
            // 'ANY' has the smallest priority
            return TLiteral::INVALID;
        }
        return TImpl::GetPriority(l);
    }
};

struct TBindBase: public TSimpleRefCount<TBindBase> {
    virtual ~TBindBase() {}
    virtual void Out(IOutputStream& out) const = 0;
};

template <class TLiteralTable, class T>
struct TBind: public TBindBase {
    const TLiteralTable& LiteralTable;
    const T& T_;
    TBind(const TLiteralTable& l, const T& t)
        : LiteralTable(l)
        , T_(t) {
    }
    void Out(IOutputStream& out) const override {
        Print(out, LiteralTable, T_);
    }
};

struct TBindWrapper {
    TIntrusivePtr<TBindBase> Ptr;
    TBindWrapper(TIntrusivePtr<TBindBase> ptr)
        : Ptr(ptr) {
    }
    void Out(IOutputStream& out) const {
        Ptr->Out(out);
    }
};

template <class TLT, class T>
inline const TBindWrapper Bind(const TLT& l, const T& t) {
    return TBindWrapper(new TBind<TLT, T>(l, t));
}

} // NRemorph

Y_DECLARE_PODTYPE(NRemorph::TLiteral);

template<>
struct THash<NRemorph::TLiteral> {
    inline size_t operator()(NRemorph::TLiteral s) const {
        return s.Value;
    }
};

template<>
inline void Out<NRemorph::TBindWrapper>(IOutputStream& out, const NRemorph::TBindWrapper& b) {
    b.Out(out);
}
