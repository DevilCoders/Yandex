#pragma once

#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/string/printf.h>

namespace NSegm {

#pragma pack(push, 1)
struct TAlignedPosting {
    TPosting Pos;

    TAlignedPosting(ui16 sent, ui16 word)
        : Pos()
    {
        Set(sent, word);
    }

    TAlignedPosting(TPosting pos = 0)
        : Pos(TWordPosition::DocLength(pos))
    {
    }

    void Set(ui16 sent, ui16 word) {
        SetPosting(Pos, sent, word);
    }

    ui16 Sent() const {
        return (ui16)TWordPosition::Break(Pos);
    }

    ui16 Word() const {
        return (ui16)TWordPosition::Word(Pos);
    }

    TAlignedPosting ToSentBegin() const {
        return IsFirstWord() ? *this : NextSent();
    }

    bool IsFirstWord() const {
        return Word() <= TWordPosition::FIRST_CHILD;
    }

    TAlignedPosting NextWord() const {
        TWordPosition p(Pos);
        p.Inc();
        return (TPosting)p.Pos;
    }

    TAlignedPosting NextSent() const {
        if (Sent() >= BREAK_LEVEL_Max)
            return *this;

        return TAlignedPosting(Sent() + 1, TWordPosition::FIRST_CHILD);
    }

    TPosting SentAligned() const {
        return ToSentBegin();//(Pos & (~INT_N_MAX(BREAK_LEVEL_Shift))) | (1 << WORD_LEVEL_Shift);
    }

    TPosting WordAligned() const {
        return Pos & (~INT_N_MAX(WORD_LEVEL_Shift));
    }

    operator TPosting() const {
        return WordAligned();
    }

    TString ToString() const {
        return Sprintf("%u.%u", Sent(), Word());
    }
};

struct TBaseSpan {
    TAlignedPosting Begin;
    TAlignedPosting End;

    explicit TBaseSpan(TAlignedPosting begin = 0, TAlignedPosting end = 0)
        : Begin(begin)
        , End(end)
    {
    }

    ui32 Sentences() const {
        return End.Sent() - Begin.Sent();
    }

    bool ContainsInEnd(TAlignedPosting last) const {
        return last > Begin && last <= End;
    }

    bool Contains(TAlignedPosting pos) const {
        return pos >= Begin && pos < End;
    }

    bool ContainsSent(ui16 sent) const {
        return sent >= Begin.Sent() && sent < End.Sent();
    }

    bool Empty() const {
        return Begin >= End;
    }

    bool NoSents() const {
        return Begin.Sent() >= End.Sent();
    }
};

struct TSpan: public TBaseSpan {
public:
    explicit TSpan(TAlignedPosting begin = 0, TAlignedPosting end = 0)
        : TBaseSpan(begin, end)
    {
    }

    friend bool operator <(const TSpan& a, const TSpan& b) {
        return a.Begin < b.Begin;
    }

    void MergePrev(const TSpan& prev) {
        Begin = prev.Begin;
    }

    void MergeNext(const TSpan& next) {
        End = next.End;
    }
};
#pragma pack(pop)

typedef TVector<TSpan> TSpans;

struct TTypedSpan : public TSpan {
    enum ESpanType {
        ST_LIST,
        ST_LIST_ITEM
    };
    ESpanType Type;
    size_t Depth;

    explicit TTypedSpan(ESpanType type, const size_t depth, TAlignedPosting begin = 0, TAlignedPosting end = 0)
        : TSpan(begin, end)
        , Type(type)
        , Depth(depth)
    {}
};

typedef TVector<TTypedSpan> TTypedSpans;

struct TCoord {
    ptrdiff_t Begin;
    ptrdiff_t End;

public:
    explicit TCoord(ptrdiff_t begin = 0, ptrdiff_t end = 0)
        : Begin(begin)
        , End(end)
    {
    }

    ptrdiff_t Size() const {
        return End - Begin;
    }

    template<typename TCharType>
    static bool Equals(const TCharType* base, const TCoord& a, const TCoord& b) {
        if (a.Size() != b.Size())
            return false;

        const TCharType* ab = base + a.Begin;
        const TCharType* bb = base + b.Begin;
        const size_t sz = a.Size();
        for (size_t off = 0; off < sz; ++off)
            if (*(ab + off) != *(bb + off))
                return false;

        return true;
    }
};

struct TPosCoord: TCoord {
    TAlignedPosting Pos;

public:
    explicit TPosCoord(TPosting pos = 0, ptrdiff_t beg = 0, ptrdiff_t end = 0)
        : TCoord(beg, end)
        , Pos(pos)
    {
    }
};

template <bool ByBegin>
struct TCoordCmp {
    bool operator()(const TCoord& a, const TCoord& b) const {
        return ByBegin ? a.Begin < b.Begin : a.End < b.End;
    }
};

using TPosCoords = TVector<TPosCoord>;

}
