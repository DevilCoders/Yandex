#pragma once

#include <util/generic/string.h>

namespace NSearchQuery {

// TRange represents interval [Beg, End].
// Special case is empty interval, for which Beg=1 and End=0. This is the only case where Beg > End
struct TRange {
    ui32 Beg;
    ui32 End;

    TRange() {
        Reset();
    }

    TRange(ui32 b, ui32 e)
        : Beg(b)
        , End(e)
    {
    }

    size_t Size() const {
        return End + 1 - Beg;
    }

    bool operator == (const TRange& rhs) const {
       return Beg == rhs.Beg && End == rhs.End;
    }

    bool operator != (const TRange& rhs) const {
       return !(*this == rhs);
    }

    bool Contains(size_t pos) const {
        return Beg <= pos && pos <= End;
    }

    bool Contains(const TRange& rhs) const {
        return Beg <= rhs.Beg && rhs.End <= End;
    }

    void Shift(int shift) {
        if (Empty())
            return;
        if (shift < 0 && (size_t)-shift > Beg)
            shift = -(int)Beg;
        Beg += shift;
        End += shift;
    }

    // Makes empty interval
    void Reset() {
        Beg = 1;
        End = 0;
    }

    bool Empty() const {
        return Beg > End;
    }

    bool operator!() const {
        return Empty();
    }

    TRange Intersect(const TRange& rhs) const {
        TRange result;
        if (End < rhs.Beg || rhs.End < Beg)
            return result;
        result.Beg = Max(Beg, rhs.Beg);
        result.End = Min(End, rhs.End);
        return result;
    }
};

inline bool RangeContains(const TRange& r, size_t pos) {
    return r.Contains(pos);
}

inline size_t GetRangeLength(const TRange& r) {
    return r.Size();
}

inline void ShiftRange(TRange& r, int shift) {
    Y_ASSERT(-shift <= (int)r.Beg);
    r.Shift(shift);
}

inline bool operator < (const TRange& a, const TRange& b) {
    if (a.Beg < b.Beg)
        return true;
    if (a.Beg == b.Beg)
        return a.End < b.End;
    return false;
}

enum EMarkupType {
    MT_SYNONYM = 0,
    MT_ONTO = 1,
    MT_FIO = 2,
    MT_SOCIAL = 3,
    MT_PROTOBUF = 4,    // obsolete
    MT_GAZETTEER = 5,   // obsolete
    MT_CITY = 6,        // obsolete
    MT_GEOADDR = 7,
    MT_SYNTAX = 8,
    MT_TECHNICAL_SYNONYM = 9,
    MT_MANGO = 10,      // obsolete
    MT_YARI = 11,
    MT_DATE = 12,
    MT_VINS = 13,
    MT_QSEGMENTS = 14,
    MT_TRANSPORT = 15,  // obsolete
    MT_TRANSIT = 16,
    MT_URL = 17,
    MT_LIST_SIZE
};

}

const TString& ToString(NSearchQuery::EMarkupType);
bool FromString(const TString& name, NSearchQuery::EMarkupType& ret);
