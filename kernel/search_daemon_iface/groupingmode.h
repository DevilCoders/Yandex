#pragma once

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/ysaveload.h>

enum EGroupingMode {
    GM_UNKNOWN = -1,
    GM_FLAT = 0,
    GM_DEEP = 1,
    GM_WIDE = 2,
    GM_DUMMY = 3,
    GM_UNION = 4,
    GM_MODES_COUNT = 5
};

struct TGroupingIndex {
    const EGroupingMode Mode;
    const TString Attr;

    TGroupingIndex(EGroupingMode g = GM_UNKNOWN, const TString& a = TString())
        : Mode(g)
        , Attr(a)
        , Hash_(CalcHash())
        , Valid_(CalcValid())
    {
    }

    TGroupingIndex(const TGroupingIndex& other) = default;

    TGroupingIndex& operator=(const TGroupingIndex& other) {
        const_cast<EGroupingMode&>(Mode) = other.Mode;
        const_cast<TString&>(Attr) = other.Attr;
        const_cast<size_t&>(Hash_) = other.Hash_;
        const_cast<bool&>(Valid_) = other.Valid_;
        return *this;
    }

    inline bool Empty() const noexcept {
        return !Attr || Mode == GM_FLAT;
    }
    inline size_t Hash() const noexcept {
        return Hash_;
    }
    inline bool Valid() const noexcept {
        return Valid_;
    }
    inline bool operator==(const TGroupingIndex& other) const noexcept {
        return Mode == other.Mode && Attr == other.Attr;
    }
    inline bool operator<(const TGroupingIndex& other) const noexcept {
        return Mode < other.Mode || (Mode == other.Mode && Attr < other.Attr);
    }

    inline size_t CalcHash() const noexcept {
        return Mode + ComputeHash(Attr);
    }
    inline bool CalcValid() const noexcept {
        return ((Mode == GM_FLAT || Mode == GM_DUMMY) && !Attr) ||
            (Mode == GM_DEEP || Mode == GM_WIDE || Mode == GM_UNION) && !!Attr;
    }

    const size_t Hash_;
    const bool Valid_;
};

struct TGroupingIndexHash {
    inline size_t operator() (const TGroupingIndex& x) const noexcept {
        return x.Hash();
    }
};

template <>
class TSerializer<TGroupingIndex> {
    public:
        static void Save(IOutputStream* rh, const TGroupingIndex& t) {
            ::Save(rh, t.Mode);
            ::Save(rh, t.Attr);
        }

        static void Load(IInputStream* rh, TGroupingIndex& t) {
            ::Load(rh, const_cast<EGroupingMode&>(t.Mode));
            ::Load(rh, const_cast<TString&>(t.Attr));
            const_cast<size_t&>(t.Hash_) = t.CalcHash();
            const_cast<bool&>(t.Valid_) = t.CalcValid();
        }

        template <class TStorage>
        static void Load(IInputStream* rh, TGroupingIndex& t, TStorage&) {
            ::Load(rh, const_cast<EGroupingMode&>(t.Mode));
            ::Load(rh, const_cast<TString&>(t.Attr));
            const_cast<size_t&>(t.Hash_) = t.CalcHash();
            const_cast<bool&>(t.Valid_) = t.CalcValid();
        }
};
