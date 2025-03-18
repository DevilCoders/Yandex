#pragma once

#include <util/system/defaults.h>

#include <cassert>
#include <util/generic/noncopyable.h>

template <class THitIter>
struct TIterLess {
    bool operator()(const THitIter* x, const THitIter* y) const {
        return x->Current() < y->Current();
    }
};

struct TIterLessShifted {
    template <class THitIter>
    bool operator()(const THitIter* x, const THitIter* y) const {
        return x->GetShiftedValue() < y->GetShiftedValue();
    }
};

struct TGetCurrentPerst {
    template <class THitIter>
    ui64 operator()(const THitIter* x) const {
        return x->GetCurrentPerst();
    }
};

struct TGetLastPerst {
    template <class THitIter>
    ui64 operator()(const THitIter* x) const {
        return x->GetLastPerst();
    }
};

template <class THitIter, class CompareIter = TIterLess<THitIter>>
class MultHitHeap : TNonCopyable {
protected:
    THitIter** HitIters; //-- the class doesn't own them!
    ui32 Total;
    ui32 Count;
    CompareIter Compare;

public:
    MultHitHeap()
        : HitIters(nullptr)
        , Total(0)
        , Count(0xfffffff)
    {
    }
    MultHitHeap(THitIter** iters, ui32 n)
        : HitIters(iters)
        , Total(n)
        , Count(0xffffffff) // call Restart() before usage
    {
// requirements like "call Restart() before any MultHitHeap usage" should be checked somehow
// "!!!" in the comment is not enough :)
#if 0 //!!!
        Restart();
#endif
    }

    virtual ~MultHitHeap() = default;

    using value_type = typename THitIter::value_type;

    bool CheckTopIter(const value_type value) const {
        if (Count > 1 && HitIters[1]->Current() < value)
            return false;
        if (Count > 2 && HitIters[2]->Current() < value)
            return false;
        return true;
    }

    template <class T>
    ui64 GetValue(const T& functor) const {
        ui64 value = 0;
        for (ui32 i = 0; i < Total; ++i)
            value += functor(HitIters[i]);
        return value;
    }

    Y_FORCE_INLINE
    bool Valid() const {
        assert(Count != 0xffffffff);
        return Count != 0;
    }

    ui32 GetValidIterCount() const {
        return Count;
    }

    MultHitHeap& operator++() {
        assert(Count != 0 && Count != 0xffffffff);
        Next();
        return *this;
    }

    inline value_type Current() const {
        assert(Count != 0 && Count != 0xffffffff);
        return HitIters[0]->Current();
    }

    inline const value_type operator*() {
        assert(Count != 0 && Count != 0xffffffff);
        return HitIters[0]->operator*();
    }

    virtual void Restart();
    void Restart(THitIter** iters, ui32 n) {
        HitIters = iters;
        Total = n;
        Count = 0xffffffff;
        Restart();
    }

    THitIter* TopIter() const {
        assert(Count != 0 && Count != 0xffffffff);
        return HitIters[0];
    }

    void SkipTo(value_type to);
    inline void Next();
    inline ui32 CheckHeap();

    inline void SiftTopIter() {
        CheckIter(0);
        Sift(0, Count);
    }

    friend bool operator<(const MultHitHeap& lh, const MultHitHeap& rh) {
        return lh.Valid() && (!rh.Valid() || lh.Current() < rh.Current());
    }

protected:
    inline ui32 CheckIter(ui32 i);
    inline void CheckIters();
    inline void Sift(ui32 node, ui32 end);
    inline void SiftAll();
};

template <class THitIter, class CompareIter>
inline void MultHitHeap<THitIter, CompareIter>::Next() {
    // assert(Count); //
    HitIters[0]->operator++();
    CheckIter(0);
    Sift(0, Count);
}

template <class THitIter, class CompareIter>
inline ui32 MultHitHeap<THitIter, CompareIter>::CheckIter(ui32 i) {
    assert(i < Count);
    if (!HitIters[i]->Valid()) {
        THitIter* hi = HitIters[i];
        HitIters[i] = HitIters[--Count];
        HitIters[Count] = hi;
        return 0;
    }
    return 1;
}

template <class THitIter, class CompareIter>
inline void MultHitHeap<THitIter, CompareIter>::CheckIters() {
    ui32 i = 0;
    while (i < Count) {
        CheckIter(i);
        if (HitIters[i]->Valid())
            ++i;
    }
}

template <class THitIter, class CompareIter>
inline void MultHitHeap<THitIter, CompareIter>::Sift(ui32 node, ui32 end) {
    ui32 son;

    THitIter* x = HitIters[node];
    for (son = 2 * node + 1; son < end; node = son, son = 2 * node + 1) {
        if (son < (end - 1) && Compare(HitIters[son + 1], HitIters[son]))
            ++son;
        if (Compare(HitIters[son], x))
            HitIters[node] = HitIters[son];
        else
            break;
    }
    HitIters[node] = x;
}

template <class THitIter, class CompareIter>
inline void MultHitHeap<THitIter, CompareIter>::SiftAll() {
    ui32 left = Count / 2;
    while (left)
        Sift(--left, Count);
}

template <class THitIter, class CompareIter>
void MultHitHeap<THitIter, CompareIter>::Restart() {
    Count = Total;
    for (ui32 i = 0; i < Count; ++i)
        HitIters[i]->Restart();
    CheckIters();
    SiftAll();
}

template <class THitIter, class CompareIter>
void MultHitHeap<THitIter, CompareIter>::SkipTo(value_type to) {
    while (Valid() && Current() < to) {
        TopIter()->SkipTo(to);
        CheckIter(0);
        Sift(0, Count);
    }
}

template <class THitIter, class CompareIter>
inline ui32 MultHitHeap<THitIter, CompareIter>::CheckHeap() {
    ui32 top, son;

    for (top = 0; top < Count; ++top) {
        son = 2 * top + 1;
        if (son < Count && Compare(HitIters[son], HitIters[top]))
            return son;
        ++son;
        if (son < Count && Compare(HitIters[son], HitIters[top]))
            return son;
    }
    return 0;
}
