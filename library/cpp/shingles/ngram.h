#pragma once

#include <util/system/defaults.h>
#include <util/system/yassert.h>

template <size_t N, typename T>
class TNGram {
    T NGram[N];
    size_t Curr;
    size_t Size;

public:
    TNGram()
        : Curr(0)
        , Size(0)
    {
    }

    size_t Update(T v) {
        size_t curr = Curr;
        NGram[Curr] = v;
        ++Curr;
        if (Curr == N)
            Curr = 0;
        if (Size < N)
            ++Size;
        return curr;
    }
    // Returns n-gram element, that will be updated the next time Update() is called
    T Get() const {
        return NGram[Curr];
    }

    size_t GetSize() const {
        return Size;
    }

    const T& operator[](size_t i) const {
        Y_ASSERT(i < Size);
        return NGram[i];
    }

    bool IsFull() const {
        return Size == N;
    }

    void Reset() {
        Curr = 0;
        Size = 0;
    }
};

template <size_t N, typename T, typename TLess>
class TNGramExtremum {
    size_t Idx;
    TNGram<N, T> NGram;

private:
    size_t FindExtremum() const {
        size_t i = Idx;
        for (size_t j = 1, n = NGram.GetSize(); j < n; ++j) {
            size_t k = Idx + j;
            if (k >= n)
                k -= n;
            if (TLess()(NGram[k], NGram[i]))
                i = k;
        }
        return i;
    }

public:
    TNGramExtremum() {
        Reset();
    }

    void Reset() {
        Idx = 0;
        NGram.Reset();
    }

    bool IsFull() const {
        return NGram.IsFull();
    }

    // Returns true if minimum has changed
    bool Update(const T& v) {
        size_t i = NGram.Update(v);
        if (!IsFull() || i != Idx) {
            if (i == Idx || TLess()(NGram[i], NGram[Idx])) {
                Idx = i;
                return true;
            }
        } else {
            Idx = FindExtremum();
            return true;
        }
        return false;
    }

    const T& Get() const {
        return NGram[Idx];
    }
};
