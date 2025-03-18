#pragma once

#include "shingler.h"

#include <util/generic/string.h>
#include <library/cpp/charset/wide.h>
#include <util/stream/output.h>

template <typename T>
struct TReadable {
    typedef T TValue;

    TValue Value;
    TUtf16String Text;

    TReadable()
        : Value(0)
    {
    }

    TReadable(const TValue& val, const TUtf16String& text)
        : Value(val)
        , Text(text)
    {
    }

    bool operator<(const TReadable<T>& r) const {
        return Value < r.Value;
    }

    bool operator==(const TReadable<T>& r) const {
        return Value == r.Value;
    }

    operator TValue() const {
        return Value;
    }

    TString ToString() const {
        return WideToUTF8(Text);
    }
};

template <class T>
static inline IOutputStream& operator<<(IOutputStream& out, const TReadable<T>& r) {
    out << r.Value << "\t\"" << r.ToString() << "\"";
    return out;
}

// Carver for readable shingles
template <size_t N, typename HasherType>
struct TWideReadableCarver {
    static const size_t NumWords = N;

    typedef HasherType THasher;
    typedef typename THasher::TValue TValue;
    typedef TReadable<TValue> TShingle;

    static TShingle CutTail(const TShingle& body, THashedWordPos<TValue> tail) {
        Y_ASSERT(tail.Num < body.Text.size());
        return TShingle(CutTailImpl<N>(body.Value, tail.Hash), TUtf16String(body.Text.data() + tail.Num + 1));
    }

    static TShingle AddHead(const TShingle& body, THashedWordPos<TValue> head, const wchar16* text) {
        return TShingle(AddHeadImpl(body.Value, head.Hash), body.Text + ' ' + TUtf16String(text + head.Pos, head.Num));
    }
};

template <size_t N, typename HasherType>
struct TYandReadableCarver: public TWideReadableCarver<N, HasherType> {
    typedef TWideReadableCarver<N, HasherType> TBase;
    typedef typename TBase::TValue TValue;
    typedef typename TBase::THasher THasher;
    typedef typename TBase::TShingle TShingle;

    using TBase::AddHead;

    static TShingle AddHead(const TShingle& body, THashedWordPos<TValue> head, const char* text) {
        return TShingle(AddHeadImpl(body.Value, head.Hash), body.Text + ' ' + CharToWide(TStringBuf(text + head.Pos, head.Len), csYandex));
    }
};

template <size_t N, typename HasherType>
struct TUTF8ReadableCarver: public TWideReadableCarver<N, HasherType> {
    typedef TWideReadableCarver<N, HasherType> TBase;
    typedef typename TBase::TValue TValue;
    typedef typename TBase::THasher THasher;
    typedef typename TBase::TShingle TShingle;

    using TBase::AddHead;

    static TShingle AddHead(const TShingle& body, THashedWordPos<TValue> head, const char* text) {
        return TShingle(AddHeadImpl(body.Value, head.Hash), body.Text + ' ' + UTF8ToWide(text + head.Pos, head.Len));
    }
};

using TReadableYandShingler = TShingler<TYandReadableCarver<5, THashFirstYandWord<TFNVHash<ui64>>>>;
using TReadableFilteredYandShingler = TShingler<TYandReadableCarver<5, THashFirstYandWord<TFNVHash<ui64>>>, TMinModInWindow<TReadable<ui64>, 10, 8>>;

using TReadableUTF8Shingler = TShingler<TUTF8ReadableCarver<5, THashFirstUTF8Word<TFNVHash<ui64>>>>;
using TReadableFilteredUTF8Shingler = TShingler<TUTF8ReadableCarver<5, THashFirstUTF8Word<TFNVHash<ui64>>>, TMinModInWindow<TReadable<ui64>, 10, 8>>;
