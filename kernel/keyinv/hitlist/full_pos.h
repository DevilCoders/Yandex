#pragma once

#include <library/cpp/wordpos/wordpos.h>

#include <util/memory/pool.h>
#include <util/ysaveload.h>

#define INVALID_POSTING (i32(0x7fffffff))

using TSignedPosting = i32;

struct TFullPosition {
    using T = TSignedPosting;
    T Beg, End;

    TFullPosition() noexcept = default;
    constexpr TFullPosition(T beg, T end) noexcept
        : Beg(beg)
        , End(end)
    {
    }
    Y_FORCE_INLINE void Clear(i32 clear = 0) {
        Beg = clear;
        End = clear;
    }
    Y_FORCE_INLINE TFullPosition& operator = (const TFullPosition&) = default;

    static Y_FORCE_INLINE EFormClass FormClass(TSignedPosting end) {
        return static_cast<EFormClass>(TWordPosition::Form(end));
    }

    Y_FORCE_INLINE EFormClass FormClass() const {
        return FormClass(End);
    }

    bool IsPartOfPhraseMatch() const {
        return (TWordPosition::Form(Beg) & 1) == 0;  // TRestrictedHeapAdapterDict449 sets the flag
    }

    void Save(IOutputStream* f) const {
        ::Save(f, Beg);
        ::Save(f, End);
    }
    void Load(IInputStream* rh) {
        ::Load(rh, Beg);
        ::Load(rh, End);
    }
};

struct TFullPositionEx {
    TFullPosition Pos;
    int WordIdx;

    void Save(IOutputStream* f) const {
        ::Save(f, Pos);
        ::Save(f, WordIdx);
    }
    void Load(IInputStream* rh) {
        ::Load(rh, Pos);
        ::Load(rh, WordIdx);
    }
};

struct TFullPositionInfo {
    TFullPosition Pos;
    TFullPosition Abs;
};

extern TFullPosition invalidPosition;

Y_FORCE_INLINE bool operator==(const TFullPosition &a, const TFullPosition &b) {
    return a.Beg == b.Beg && a.End == b.End;
}

Y_FORCE_INLINE bool operator!=(const TFullPosition &a, const TFullPosition &b) {
    return a.Beg != b.Beg || a.End != b.End;
}

Y_FORCE_INLINE bool operator<(const TFullPosition &a, const TFullPosition &b) {
    return a.Beg < b.Beg;
}

Y_FORCE_INLINE bool operator<(const TFullPositionEx &a, const TFullPositionEx &b) {
    if (a.Pos.Beg != b.Pos.Beg)
        return a.Pos.Beg < b.Pos.Beg;
    else
        return a.WordIdx < b.WordIdx;
}

