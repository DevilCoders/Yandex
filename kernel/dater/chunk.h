#pragma once

#include "pattern_traits.h"
#include "document.h"

#include <library/cpp/langs/langs.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/system/yassert.h>

namespace ND2 {
namespace NImpl {

const i32 MAX_SPANS_IN_CHUNK = 32;

static const ui32 MAX_DATE_TIME_DIST = 12;

static const ui32 MAX_SCANS = 1024;
static const ui32 MAX_CHUNKS_IN_ALL_SCANS   = 4048;
static const ui32 MAX_CHUNKS_IN_SINGLE_SCAN = 256;

enum ESpanSemantics {
    SS_NONE = 0,

    SS_ID    = 0x1,
    SS_YEAR  = 0x2,
    SS_MONTH = 0x4,
    SS_DAY   = 0x8,

    SS_DATE = SS_DAY | SS_MONTH | SS_YEAR,

    SS_NUM = 0x100,
    SS_WRD  = 0x200,

    SS_TIME = 0x400,
    SS_JUNK = 0x800,

    SS_SOME_MEANING = SS_ID | SS_DATE | SS_TIME | SS_JUNK
};

enum EChunkSemantics {
    CS_JUNK = 0,
    CS_TIME = 1,
    CS_DATE = 2,
    CS_DATE_RANGE = 4,
    CS_ID = 5,
};

struct TSemanticItem {
    ui32 Meaning;

    TSemanticItem()
        : Meaning()
    {}

    template <ui32 t>
    bool HasMeaning() const {
        return (Meaning & t) == t;
    }

    template <ui32 t>
    bool HasSomeOfMeanings() const {
        return Meaning & t;
    }

    template <ui32 t>
    void AddMeaning() {
        Meaning |= t;
    }

    template <ui32 t>
    void RemoveMeaning() {
        Meaning &= ~t;
    }

    template <ui32 t>
    void SetMeaning() {
        Meaning = t;
    }
};

struct TChunkSpan : public TSemanticItem {
    const wchar16* Begin;
    const wchar16* End;

    ui32 Value;

    TChunkSpan()
        : Begin()
        , End()
        , Value()
    {
    }

    bool IsMeaningfull() const {
        return Meaning;
    }

    bool Empty() const {
        return !Size();
    }

    size_t Size() const {
        return End >= Begin && Begin ? End - Begin : 0;
    }

    friend bool operator<(const TChunkSpan& a, const TChunkSpan& b) {
        return !a.Begin ? (bool)b.Begin
                        : b.Begin ? a.Begin < b.Begin
                                  : false;
    }

    TString ToString() const;

    TWtringBuf Span() const {
        return TWtringBuf(Begin, End);
    }
};

struct TChunk : public TSemanticItem {
    static const ui32 NoSpan = -1;

    ELanguage  Language;
    EPatternType PatternType;

private:

    TChunkSpan  SpansArray[MAX_SPANS_IN_CHUNK + 3];
    TChunkSpan* Spans;
    i32 LastSpan;

    void SetSpans() {
        Spans = &(SpansArray[2]);
    }

public:

    TChunk()
    {
        Zero(*this);
        SetSpans();
        Reset();
    }

    TChunk(const TChunk& ch)
        : TSemanticItem(ch)
    {
        *this = ch;
    }

    TChunk& operator=(const TChunk& ch) {
        memcpy(this, &ch, sizeof(TChunk));
        SetSpans();
        return *this;
    }

    TChunkSpan& PushSpan() {
        LastSpan += bool(LastSpan < MAX_SPANS_IN_CHUNK);
        Zero(BackSpan());
        return BackSpan();
    }

    void PopSpan() {
        LastSpan -= bool(LastSpan >= 0);
    }

    TChunkSpan& PrevSpan() {
        return Spans[LastSpan - 1];
    }

    TChunkSpan& FrontSpan() {
        return Spans[0];
    }

    TChunkSpan& BackSpan() {
        return Spans[LastSpan];
    }

    TChunkSpan& GetSpan(size_t i) {
        return Spans[i];
    }

    const TChunkSpan& GetSpan(size_t i) const { return const_cast<TChunk*>(this)->GetSpan(i); }
    const TChunkSpan& FrontSpan()       const { return const_cast<TChunk*>(this)->FrontSpan(); }
    const TChunkSpan& BackSpan()        const { return const_cast<TChunk*>(this)->BackSpan(); }

    TChunkSpan&       operator[] (size_t i)       { return GetSpan(i); }
    const TChunkSpan& operator[] (size_t i) const { return GetSpan(i); }

    ui32 SpanCount() const {
        return Max<i32>(LastSpan + 1, 0);
    }

    void Reset(EChunkSemantics cs, ESpanSemantics ss) {
        const wchar16* a[2] = { Begin(), End() };
        Reset();
        PushSpan();
        Spans[0].Begin = a[0];
        Spans[0].End = a[1];
        Spans[0].Meaning = ss;
        Meaning = cs;
    }

    bool NoSpans() const {
        return !SpanCount();
    }

    void Reset() {
        LastSpan = -1;
    }

    const wchar16* Begin() const { return NoSpans() ? nullptr : FrontSpan().Begin; }
    const wchar16* End()   const { return NoSpans() ? nullptr : BackSpan().End; }

    size_t Size() const {
        const wchar16* b = Begin();
        const wchar16* e = End();
        return b && e ? e - b : 0;
    }

    const wchar16* MeaningfullBegin() const {
        for (i32 i = 0; i <= LastSpan; ++i)
            if (Spans[i].HasSomeOfMeanings<SS_SOME_MEANING>())
                return Spans[i].Begin;
        return nullptr;
    }

    const wchar16* MeaningfullEnd() const {
        for (i32 i = LastSpan; i >=0; --i)
            if (Spans[i].HasSomeOfMeanings<SS_SOME_MEANING>())
                return Spans[i].End;
        return nullptr;
    }

    size_t MeaningfullSize() const {
        const wchar16* b = MeaningfullBegin();
        const wchar16* e = MeaningfullEnd();
        return b && e ? e - b : 0;
    }

    bool IsMeaningfull() const {
        for (i32 i = 0; i <= LastSpan; ++i)
            if (Spans[i].IsMeaningfull())
                return true;
        return false;
    }

    template <ui32 t>
    ui32 FindFirstOf(i32 end = MAX_SPANS_IN_CHUNK + 1, i32 beg = -1) const {
        for (i32 i = Max<i32>(beg + 1, 0); i <= Min<i32>(end - 1, LastSpan); ++i) {
            if (Spans[i].HasSomeOfMeanings<t>())
                return i;
        }

        return NoSpan;
    }

    template <ui32 t>
    ui32 FindLastOf(i32 beg = -1, i32 end = MAX_SPANS_IN_CHUNK + 1) const {
        for (i32 i = Min<i32>(end - 1, LastSpan); i >= Max<i32>(beg + 1, 0); --i) {
            if (Spans[i].HasSomeOfMeanings<t>())
                return i;
        }

        return NoSpan;
    }

    friend bool operator<(const TChunk& a, const TChunk& b) {
        return a.NoSpans() ? !b.NoSpans()
                           :  b.NoSpans() ? false
                                          : a[0] < b[0];
    }

    TString ToString() const;
};

typedef TVector<TChunk> TChunks;

}
}

static inline IOutputStream& operator<<(IOutputStream& out, const ND2::NImpl::TChunk& ch) {
    return out << ch.ToString();
}
