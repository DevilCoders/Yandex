#pragma once

#include "hash_func.h"
#include "word_hash.h"
#include "ngram.h"

#include <util/generic/vector.h>

template <typename T>
struct TFakeFilter {
    T Value;

    bool Update(const T& val) {
        if (!val)
            return false;
        Value = val;
        return true;
    }

    const T& Get() const {
        return Value;
    }

    void Reset() {
        return;
    }
};

template <size_t M>
struct TModPow2Less {
    static const ui64 Mask = ~(((ui64)-1) << M);

    bool operator()(ui64 a, ui64 b) const {
        return (a & Mask) < (b & Mask);
    }
};

template <typename T, size_t N, size_t M>
struct TMinModInWindow {
    TNGramExtremum<N, T, TModPow2Less<M>> Minimum;

    bool Update(const T& val) {
        if (!val)
            return false;

        const bool wasFull = Minimum.IsFull();
        const bool newMin = Minimum.Update(val);
        const bool isFull = Minimum.IsFull();

        // Test is passed if we have just reached ngram size or if minimum has changed
        return isFull && (newMin && wasFull || !wasFull);
    }

    const T& Get() const {
        return Minimum.Get();
    }

    void Reset() {
        Minimum.Reset();
    }
};

template <size_t N, typename T>
T CutTailImpl(T body, T tail) {
    return body - (NFNV::TPrimePower<T, N - 1>::Val * tail);
}

template <typename T>
T AddHeadImpl(T body, T head) {
    return (body * NFNV::TPrime<T>::Val) + head;
}

// Carver for most basic shingle: numeric
template <size_t N, typename HasherType>
struct TBasicShingleCarver {
    static const size_t NumWords = N;

    typedef HasherType THasher;
    typedef typename THasher::TValue TValue;
    typedef TValue TShingle;

    static TShingle CutTail(TShingle body, THashedWordPos<TValue> tail) {
        return CutTailImpl<N>(body, tail.Hash);
    }

    template <typename CharType>
    static TShingle AddHead(TShingle body, THashedWordPos<TValue> head, const CharType*) {
        return AddHeadImpl(body, head.Hash);
    }
};

// Carver for shingle with positions
// Positioned shingles must be calculated from one string
template <size_t N, typename HasherType>
struct TPositionedShingleCarver {
    static const size_t NumWords = N;

    typedef HasherType THasher;
    typedef typename THasher::TValue TValue;
    typedef THashedWordPos<TValue> TShingle;

    static TShingle CutTail(TShingle body, THashedWordPos<TValue> tail) {
        Y_ASSERT(tail.Pos == body.Pos && tail.Len <= body.Len);
        return TShingle(CutTailImpl<N>(body.Hash, tail.Hash), body.Pos + tail.Len, body.Len - tail.Len, body.Num - tail.Num);
    }

    template <typename CharType>
    static TShingle AddHead(TShingle body, THashedWordPos<TValue> head, const CharType*) {
        Y_ASSERT(body.Pos <= head.Pos);
        return TShingle(AddHeadImpl(body.Hash, head.Hash), body.Pos, body.Len + head.Len, body.Num + head.Num);
    }
};

template <typename CarverType, typename FilterType = TFakeFilter<typename CarverType::TShingle>>
class TShingler {
    typedef CarverType TCarver;
    typedef FilterType TFilter;
    typedef typename TCarver::THasher THasher;
    typedef typename THasher::TResult TPart;

    template <typename T, bool IsPrimitive>
    struct TInitShingle {
        static T Do() {
            return T();
        }
    };

    template <typename T>
    struct TInitShingle<T, true> {
        static T Do() {
            return 0;
        }
    };

public:
    typedef typename TCarver::TShingle TShingle;

private:
    const size_t MinWordLength;
    const bool CaseSensitive;

    TNGram<TCarver::NumWords, TPart> Parts;
    TShingle Shingle;
    TFilter Filter;

protected:
    template <typename CharType>
    void Update(TVector<TShingle>& result, TPart part, const CharType* text) {
        if (Parts.IsFull()) {
            // 'Cut a tail' from complete shingle
            Shingle = TCarver::CutTail(Shingle, Parts.Get());
        }

        // 'Add a head' to shingle
        Shingle = TCarver::AddHead(Shingle, part, text);

        // Update parts sequence
        Parts.Update(part);

        if (Parts.IsFull()) {
            if (Filter.Update(Shingle)) {
                result.push_back(Filter.Get());
            }
        }
    }

public:
    TShingler(size_t minWordLength, bool caseSensitive)
        : MinWordLength(minWordLength)
        , CaseSensitive(caseSensitive)
        , Shingle(TInitShingle<TShingle, std::is_scalar<TShingle>::value>::Do())
    {
    }

    // Calculate shingles from given text, retrieve full filtered result (can include shingles, that were cached in filter)
    template <typename CharType>
    size_t ShingleText(TVector<TShingle>& result, const CharType* text, size_t len) {
        size_t pos = 0;
        size_t words = 0;
        while (pos < len) {
            // Get next word's hash, relative offset and length
            TPart part = THasher::Do(text + pos, len - pos, CaseSensitive);

            // Convert relative offset to absolute
            part.Pos += pos;

            // Update pos
            pos = part.Pos + part.Len;

            // For non-trivial parts update full shingle
            if (part.Num >= MinWordLength) {
                Update(result, part, text);
                ++words;
            }
        }
        return words;
    }

    // Update current shingle with boundary symbol, retrieve filtered result (can include shingles, that were cached in filter)
    void ShingleBound(TVector<TShingle>& result) {
        Update(result, TPart(THasher::THashFunc::Prime, 0, 0, 0), (const char*)nullptr);
    }

    // Reset current shingler state
    void Reset() {
        Shingle = TShingle();
        Parts.Reset();
        Filter.Reset();
    }
};

using TYandShingler = TShingler<TBasicShingleCarver<5, THashFirstYandWord<TFNVHash<ui64>>>>;
using TFilteredYandShingler = TShingler<TBasicShingleCarver<5, THashFirstYandWord<TFNVHash<ui64>>>, TMinModInWindow<ui64, 10, 8>>;

using TUTF8Shingler = TShingler<TBasicShingleCarver<5, THashFirstUTF8Word<TFNVHash<ui64>>>>;
using TFilteredUTF8Shingler = TShingler<TBasicShingleCarver<5, THashFirstUTF8Word<TFNVHash<ui64>>>, TMinModInWindow<ui64, 10, 8>>;
