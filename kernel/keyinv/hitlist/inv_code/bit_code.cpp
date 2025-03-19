#include <util/system/defaults.h>
#include <util/generic/bitops.h>
#include <util/generic/hash.h>

#include <library/cpp/wordpos/wordpos.h>
#include <library/cpp/sse/sse.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "bit_code.h"

#ifdef NDEBUG
#   define RELEASE_INLINE Y_FORCE_INLINE
#   define RELEASE_BUILD 1
#else
#   define RELEASE_INLINE
#   define RELEASE_BUILD 0
#endif

namespace Doc0 {
#include "doc_0.inc"
ui64 idtable0[256];
};

namespace Phrase0 {
#include "phrase_0d.inc"
ui64 idtable0[256];
};

namespace Phrase1 {
#include "phrase_1d.inc"
ui64 idtable0[256];
};

namespace Word0 {
#include "word_0.inc"
ui64 idtable0[256];
};

namespace Word1 {
#include "word_1.inc"
ui64 idtable0[256];
};

namespace FormRelev0 {
#include "formrelev_0.inc"
ui64 idtable0[128];
};

namespace FormRelev1 {
#include "formrelev_1.inc"
ui64 idtable0[128];
};

#if (__GNUC__ == 4 && __GNUC_MINOR__ < 3)
#    define FAKE
#endif

#if (__GNUC__ == 4 && __GNUC_MINOR__ > 3 && RELEASE_BUILD == 1 && !defined(__INTEL_COMPILER) && (defined(_i386_) || defined(_x86_64_)))
#    define OPTIMIZED  __attribute__ ((__target__ ("arch=core2,tune=core2")))
#else
#    define OPTIMIZED
#endif

#ifndef FAKE

__m128i RELEASE_INLINE SLL64(const __m128i &xmm, int value) {
    return _mm_slli_epi64(xmm, value);
}
__m128i RELEASE_INLINE SLL32(const __m128i &xmm, int value) {
    return _mm_slli_epi32(xmm, value);
}
__m128i RELEASE_INLINE SLL16(const __m128i &xmm, int value) {
    return _mm_slli_epi16(xmm, value);
}
__m128i RELEASE_INLINE SRL64(const __m128i &xmm, int value) {
    return _mm_srli_epi64(xmm, value);
}
__m128i RELEASE_INLINE SRL32(const __m128i &xmm, int value) {
    return _mm_srli_epi32(xmm, value);
}
__m128i RELEASE_INLINE SRL16(const __m128i &xmm, int value) {
    return _mm_srli_epi16(xmm, value);
}

#else

__m128i RELEASE_INLINE SLL64(const __m128i &xmm, int value) {
    return _mm_sll_epi64(xmm, _mm_cvtsi32_si128(value));
}
__m128i RELEASE_INLINE SLL32(const __m128i &xmm, int value) {
    return _mm_sll_epi32(xmm, _mm_cvtsi32_si128(value));
}
__m128i RELEASE_INLINE SLL16(const __m128i &xmm, int value) {
    return _mm_sll_epi16(xmm, _mm_cvtsi32_si128(value));
}
__m128i RELEASE_INLINE SRL64(const __m128i &xmm, int value) {
    return _mm_srl_epi64(xmm, _mm_cvtsi32_si128(value));
}
__m128i RELEASE_INLINE SRL32(const __m128i &xmm, int value) {
    return _mm_srl_epi32(xmm, _mm_cvtsi32_si128(value));
}
__m128i RELEASE_INLINE SRL16(const __m128i &xmm, int value) {
    return _mm_srl_epi16(xmm, _mm_cvtsi32_si128(value));
}


#endif

__m128i RELEASE_INLINE Integrate32(const __m128i &s0) {

    const __m128i s1 = _mm_add_epi32(_mm_slli_si128(s0, 8), s0);
    const __m128i s2 = _mm_add_epi32(_mm_slli_si128(s1, 4), s1);
    return s2;
};

__m128i RELEASE_INLINE Integrate16(const __m128i &s0) {

    const __m128i s1 = _mm_add_epi16(_mm_slli_si128(s0, 8), s0);
    const __m128i s2 = _mm_add_epi16(_mm_slli_si128(s1, 4), s1);
    const __m128i s3 = _mm_add_epi16(_mm_slli_si128(s2, 2), s2);
    return s3;
};

__m128i  masks32[32];
__m128i  masks16[32];
static const __m128i mask64 = _mm_set_epi32(0, -1, 0, -1);
i8 offset0[32];
i8 offset1[32];
i8 offset2[32];
i8 offset3[32];
i8 shift0[32];
i8 shift1[32];
i8 shift2[32];
i8 shift3[32];


ui64 Inverse(ui64 value) {
    ui64 res = 0;
    for (size_t i = 0; i < 64; i += 8)
        res |= (((value >> i) & 255) << (56 - i));
    return res;
};

ui64 InverseIntegrate(ui64 value) {
    ui64 res = 0;
    ui64 count = 0;
    for (int i = 56; i >= 0; i -= 8) {
        count += ((value >> i) & 255);
        res |= (count << (56 - i));
    }
    assert(count < 256);
    return res;
};

void Inverse(const ui64 *from, ui64 *dst0, const ui8 *itable, size_t length = 256) {
    for (size_t i = 0; i < length; ++i) {
        ui64 a = Inverse(from[i]);
        if (itable)
            a = a << itable[i];
        dst0[i] = a;
    }
}

void InverseIntegrate(const ui64 *from, ui64 *dst0, size_t length = 256) {
    for (size_t i = 0; i < length; ++i) {
        dst0[i] = InverseIntegrate(from[i]);
    }
}


struct TInit {
    TInit() {


        InverseIntegrate(Doc0::dtable, Doc0::idtable0);

        Inverse(Phrase0::dtable, Phrase0::idtable0, nullptr);
        Inverse(Word0::dtable, Word0::idtable0, Word0::itable);
        Inverse(FormRelev0::dtable, FormRelev0::idtable0, FormRelev0::itable, 128);

        Inverse(Phrase1::dtable, Phrase1::idtable0, nullptr);
        Inverse(Word1::dtable, Word1::idtable0, Word1::itable);
        Inverse(FormRelev1::dtable, FormRelev1::idtable0, FormRelev1::itable, 128);

        for (size_t i = 0; i < 32; ++i) {

            offset0[i] = 0;
            offset1[i] = (2 * i) >> 3;
            offset2[i] = (4 * i) >> 3;
            offset3[i] = (6 * i) >> 3;

            shift0[i] = 0;
            shift1[i] = (2 * i) & 7;
            shift2[i] = (4 * i) & 7;
            shift3[i] = (6 * i) & 7;

            if (i == 0) {
                offset0[i] = -1;
                offset1[i] = -1;
                offset2[i] = -1;
                offset3[i] = -1;
            }

            ui32 value = (1u << i) - 1;
            masks32[i] =  _mm_set1_epi32((1u << i) - 1);
            masks16[i] =  _mm_set1_epi32(value + (value << 16));
        }
    }
} unnamed;


void RELEASE_INLINE Shift(ui8 *data, const __m128i &value, size_t shift, size_t offset) {
    __m128i to = _mm_loadl_epi64((__m128i *)(data + offset));
    to = _mm_or_si128(to, _mm_slli_epi64(value, shift));
    _mm_storel_epi64((__m128i *)(data + offset), to);
}

RELEASE_INLINE
ui8 *Flush(const __m128i &a0, const __m128i &a1, size_t currentLevel, ui8 *data) {

    const __m128i zero = _mm_setzero_si128();

    const __m128i a0_0 = _mm_and_si128(a0, mask64);
    const __m128i a0_1 = _mm_srli_epi64(a0, 32);
    const __m128i v0 = _mm_or_si128(a0_0, _mm_slli_epi64(a0_1, currentLevel));
    const __m128i a1_0 = _mm_and_si128(a1, mask64);
    const __m128i a1_1 = _mm_srli_epi64(a1, 32);
    const __m128i v1 = _mm_or_si128(a1_0, _mm_slli_epi64(a1_1, currentLevel));
    const __m128i c0 = v0;
    const __m128i c1 = _mm_srli_si128(v0, 8);
    const __m128i c2 = v1;
    const __m128i c3 = _mm_srli_si128(v1, 8);

    if (currentLevel < 9) {
        __m128i r = c3;
        int shift = currentLevel * 2;
        r = _mm_or_si128(_mm_slli_epi64(r, shift), c2);
        r = _mm_or_si128(_mm_slli_epi64(r, shift), c1);
        r = _mm_or_si128(_mm_slli_epi64(r, shift), c0);
        _mm_storel_epi64((__m128i *)data, r);
    } else if (currentLevel < 17) {
        int shift = currentLevel * 2;
        _mm_storeu_si128((__m128i *)(data + 0), zero);
        _mm_storel_epi64((__m128i *)data, _mm_or_si128(_mm_slli_epi64(c1, shift), c0));
        Shift(data, _mm_or_si128(_mm_slli_epi64(c3, shift), c2), shift2[currentLevel], offset2[currentLevel]);
    } else {
        _mm_storeu_si128((__m128i *)(data + 0), zero);
        _mm_storeu_si128((__m128i *)(data + 16), zero);
        _mm_storel_epi64((__m128i *)data, c0);
        Shift(data, c1, shift1[currentLevel], offset1[currentLevel]);
        Shift(data, c2, shift2[currentLevel], offset2[currentLevel]);
        Shift(data, c3, shift3[currentLevel], offset3[currentLevel]);
    }

    return data + currentLevel;
}


template<size_t Shift, ui64 HashMul>
struct TPerfectHash {
    ui64 HashValues[1 << Shift];
    ui8 Indices[1 << Shift];
    ui8 Levels[1 << Shift];
    Y_FORCE_INLINE
    ui64 HashValue(ui64 data, ui64 level) {
        return (data * HashMul + (level << 56)) >> (64 - Shift);
    }
    TPerfectHash(const ui64 *dataTable, const ui8 *indexTable, size_t size) {
        memset(Indices, 0xff, sizeof(Indices));
        memset(Levels, 0xff, sizeof(Levels));
        memset(HashValues, 0xff, sizeof(HashValues));
        for (size_t i = 0; i < size; ++i) {
            ui64 hashValue = HashValue(dataTable[i], indexTable[i]);
            assert(Indices[hashValue] == 0xff);
            Indices[hashValue] = i;
            Levels[hashValue] = indexTable[i];
            HashValues[hashValue] = dataTable[i];
        }
    }
};

static const __m128i mask15Not = _mm_set1_epi32(~((1 << 15) - 1));

template<size_t Bits, size_t Shift, ui64 HashMul, size_t BitOverhead>
struct TCompressor {
    __m128i Mask;
    TPerfectHash<Shift, HashMul> Hash;
    ui64 Base;


    TCompressor(const ui64 *dataTable, const ui8 *indexTable, size_t size)
        : Hash(dataTable, indexTable, size)
    {
        ui16 value = ~((1 << BitOverhead) - 1);
        Mask = _mm_set1_epi32(value + (((ui32)value) << 16));
    }

    template<bool Large> RELEASE_INLINE
    ui64 FitByte(__m128i slice0, __m128i slice1, size_t &shift) {
        shift = 0;
        slice1 = _mm_shuffle_epi32(slice1, _MM_SHUFFLE(0, 1, 2, 3));
        slice0 = _mm_shuffle_epi32(slice0, _MM_SHUFFLE(0, 1, 2, 3));
        while (Large) {
            const __m128i value = _mm_cmpeq_epi8(_mm_and_si128(_mm_or_si128(slice0, slice1), mask15Not), _mm_setzero_si128());
            if (_mm_movemask_epi8(value) == 0xffff)
                break;
            slice0 = _mm_srli_epi32(slice0, 8);
            slice1 = _mm_srli_epi32(slice1, 8);
            shift += 8;
        }

        __m128i res16 = _mm_packs_epi32(slice1, slice0);
        while (BitOverhead) {
            const __m128i value = _mm_cmpeq_epi8(_mm_and_si128(res16, Mask), _mm_setzero_si128());
            if (_mm_movemask_epi8(value) == 0xffff)
                break;
            res16 = _mm_srli_epi16(res16, 1);
            ++shift;
        }

        const __m128i res8 = _mm_packus_epi16(res16, _mm_setzero_si128());
#if defined __x86_64__ || defined _ppc64_
        return _mm_cvtsi128_si64(res8);
#else
        ui64 data[1];
        _mm_storel_epi64((__m128i *)data, res8);
        return data[0];
#endif
    }

    template<bool Large> RELEASE_INLINE
    ui8 GetCode(__m128i &v0, __m128i &v1, ui8 &code) {
        size_t shift;
        ui64 lookup = FitByte<Large>(v0, v1, shift);
        for (size_t i = shift; i < Bits + 1; ++i) {
            ui64 it = Hash.HashValue(lookup, i);
            if (Hash.HashValues[it] == lookup) {
                code = Hash.Indices[it];
                v0 = _mm_and_si128(v0, masks32[i]);
                v1 = _mm_and_si128(v1, masks32[i]);
                return i;
            }
            lookup = (lookup >> 1) & ULL(0x7f7f7f7f7f7f7f7f);
        }
        assert(0);
        code = 0;
        return 0;
    }

};

static TCompressor<26, 10, ULL(0xf7d89233b14e2be4), 2> docCompressor0(Doc0::dtable, Doc0::itable, 256);
static TCompressor<15, 10, ULL(0x7cdb9de51da3866a), 4> phraseCompressor0(Phrase0::dtable, Phrase0::itable, 256);
static TCompressor<6, 10, ULL(0x0af8eb67563cdd5b), 0> wordCompressor0(Word0::dtable, Word0::itable, 256);
static TCompressor<6, 9, ULL(0xf0c283fccb2f2198), 0> formRelevCompressor0(FormRelev0::dtable, FormRelev0::itable, 128);

static TCompressor<15, 10, ULL(0x49a6ec0f450be293), 4> phraseCompressor1(Phrase1::dtable, Phrase1::itable, 256);
static TCompressor<6, 10, ULL(0x19fbceef645942cb), 0> wordCompressor1(Word1::dtable, Word1::itable, 256);
static TCompressor<6, 9, ULL(0xf01c0b33f8d0d059), 0> formRelevCompressor1(FormRelev1::dtable, FormRelev1::itable, 128);



#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4309) //'argument' : truncation of constant value (0xffff -> short)
#endif
const __m128i m32 = _mm_set_epi32(0xffffffff, 0, 0xffffffff, 0);
const __m128i m16 = _mm_set_epi16(0xffff, 0, 0xffff, 0, 0xffff, 0, 0xffff, 0);
#ifdef _MSC_VER
#pragma warning(pop)
#endif


__m128i RELEASE_INLINE Mix32(const __m128i &v, size_t shift) {

    const __m128i a = SLL64(v, 32 - shift);
    return _mm_or_si128(_mm_and_si128(m32, a), _mm_andnot_si128(m32, v));
}

__m128i RELEASE_INLINE Mix16(const __m128i &v, size_t shift) {

    const __m128i a = SLL32(v, 16 - shift);
    return _mm_or_si128(_mm_and_si128(m16, a), _mm_andnot_si128(m16, v));
}

int RELEASE_INLINE Decode(ui8 code0, const ui8 *itable, const ui64 *idtable0, __m128i &value) {
    value = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)(idtable0 + code0)), _mm_setzero_si128());
    return itable[code0];
}

template<bool Doc> OPTIMIZED RELEASE_INLINE
const ui8 *DecodeLoSIMDTemplate(int mask, const ui8 *stream, __m128i &r0, __m128i &r1) {
    __m128i formrelev;
    __m128i word;
    __m128i phrase;

    int a, b, c;

    if (Doc) {
        a = Decode(mask & 0x7f, FormRelev0::itable, FormRelev0::idtable0, formrelev);
        b = Decode(stream[1], Word0::itable, Word0::idtable0, word);
        c = Decode(stream[2], Phrase0::itable, Phrase0::idtable0, phrase);
    } else {
        a = Decode(mask & 0x7f, FormRelev1::itable, FormRelev1::idtable0, formrelev);
        b = Decode(stream[1], Word1::itable, Word1::idtable0, word);
        c = Decode(stream[2], Phrase1::itable, Phrase1::idtable0, phrase);
    }

    stream += 3;
    int ab = a + b;
    int abc = ab + c;
    if (abc <  17) {
        const __m128i v0_0 =       _mm_loadl_epi64((__m128i *)(stream + offset0[abc]));
        const __m128i v1_0 = SRL64(_mm_loadl_epi64((__m128i *)(stream + offset2[abc])), shift2[abc]);
        const __m128i v01_0 = Mix32(_mm_unpacklo_epi64(v0_0, v1_0), abc * 2);
        const __m128i v23_0 = Mix16(v01_0, abc);

        formrelev = _mm_add_epi16(formrelev, _mm_and_si128(v23_0, masks16[a]));

        word = _mm_add_epi16(word, _mm_and_si128(SRL16(v23_0, a), masks16[b]));

        phrase = _mm_add_epi16(SLL16(phrase, c), _mm_and_si128(SRL32(v23_0, ab), masks16[c]));

        __m128i frw = _mm_slli_epi16(word, 6);

        frw = _mm_add_epi16(formrelev, frw);

        const __m128i zero = _mm_setzero_si128();

        r0 = _mm_unpacklo_epi16(frw, zero);
        r0 = _mm_add_epi32(_mm_slli_epi32(_mm_unpacklo_epi16(phrase, zero), 12), r0);

        r1 = _mm_unpackhi_epi16(frw, zero);
        r1 = _mm_add_epi32(_mm_slli_epi32(_mm_unpackhi_epi16(phrase, zero), 12), r1);

    } else {
        const __m128i mab = masks32[ab];
        const __m128i mc = masks32[c];

        const __m128i v0_0 =       _mm_loadl_epi64((__m128i *)(stream));
        const __m128i v1_0 = SRL64(_mm_loadl_epi64((__m128i *)(stream + offset1[abc])), shift1[abc]);
        const __m128i v2_0 = SRL64(_mm_loadl_epi64((__m128i *)(stream + offset2[abc])), shift2[abc]);
        const __m128i v3_0 = SRL64(_mm_loadl_epi64((__m128i *)(stream + offset3[abc])), shift3[abc]);

        const __m128i v01_0 = Mix32(_mm_unpacklo_epi64(v0_0, v1_0), abc);
        const __m128i v23_0 = Mix32(_mm_unpacklo_epi64(v2_0, v3_0), abc);
        const __m128i vpacked = _mm_packs_epi32(_mm_and_si128(v01_0, mab), _mm_and_si128(v23_0, mab));

        formrelev = _mm_add_epi16(formrelev, _mm_and_si128(vpacked, masks16[a]));

        word = _mm_add_epi16(word, SRL16(vpacked, a));

        phrase = SLL16(phrase, c);

        __m128i frw = _mm_slli_epi16(word, 6);
        frw = _mm_add_epi16(formrelev, frw);

        const __m128i mc0123 = _mm_and_si128(SRL32(v01_0, ab), mc);
        r0 = _mm_slli_epi32(_mm_add_epi32(_mm_unpacklo_epi16(phrase, _mm_setzero_si128()), mc0123), 12);
        r0 = _mm_add_epi32(r0, _mm_unpacklo_epi16(frw, _mm_setzero_si128()));

        const __m128i mc4567 = _mm_and_si128(SRL32(v23_0, ab), mc);
        r1 = _mm_slli_epi32(_mm_add_epi32(_mm_unpackhi_epi16(phrase, _mm_setzero_si128()), mc4567), 12);
        r1 = _mm_add_epi32(r1, _mm_unpackhi_epi16(frw, _mm_setzero_si128()));
    }

    return stream + abc;
}

template<bool Doc> RELEASE_INLINE
const ui8 *Skip(int mask, const ui8 *stream) {

    if (Doc) {
        return stream + 3 + FormRelev0::itable[mask & 0x7f] + Word0::itable[stream[1]] + Phrase0::itable[stream[2]];
    } else {
        return stream + 3 + FormRelev1::itable[mask & 0x7f] + Word1::itable[stream[1]] + Phrase1::itable[stream[2]];
    }
}


#define OP_0_SETUP\
        const __m128i addp = _mm_loadl_epi64((__m128i *)(Doc0::idtable0 + code));\
        const __m128i v0 = _mm_unpacklo_epi8(addp, zero);

#define OP_1_SETUP\
        const __m128i ma = masks16[a];\
        const __m128i v0_0 =       _mm_loadl_epi64((__m128i *)(coded));\
        const __m128i v1_0 = SRL64(_mm_loadl_epi64((__m128i *)(coded + offset2[a])), shift2[a]);\
        const __m128i v01_0 = Mix32(_mm_unpacklo_epi64(v0_0, v1_0), a << 1);\
        const __m128i v23_0 = _mm_and_si128(Mix16(v01_0, a), ma);\
        const __m128i addp = _mm_loadl_epi64((__m128i *)(Doc0::idtable0 + code));\
        const __m128i addu = _mm_unpacklo_epi8(addp, zero);\
        const __m128i v0 = _mm_add_epi16(Integrate16(v23_0), SLL16(addu, a));

#define OP_2_SETUP\
    const __m128i ma = masks32[a];\
        const __m128i v0_0 =       _mm_loadl_epi64((__m128i *)(coded));\
        const __m128i v1_0 = SRL64(_mm_loadl_epi64((__m128i *)(coded + offset1[a])), shift1[a]);\
        const __m128i v01_0 = _mm_and_si128(Mix32(_mm_unpacklo_epi64(v0_0, v1_0), a), ma);\
        const __m128i s0 = _mm_add_epi32(Integrate32(v01_0), docId);\
        const __m128i v2_0 = SRL64(_mm_loadl_epi64((__m128i *)(coded + offset2[a])), shift2[a]);\
        const __m128i v3_0 = SRL64(_mm_loadl_epi64((__m128i *)(coded + offset3[a])), shift3[a]);\
        const __m128i v23_0 = _mm_and_si128(Mix32(_mm_unpacklo_epi64(v2_0, v3_0), a), ma);\
        const __m128i s1 = _mm_add_epi32(Integrate32(v23_0), _mm_shuffle_epi32(s0, _MM_SHUFFLE(3, 3, 3, 3)));\
        const __m128i addp = _mm_loadl_epi64((__m128i *)(Doc0::idtable0 + code));\
        const __m128i addu = _mm_unpacklo_epi8(addp, zero);\
        const __m128i add0 = _mm_unpacklo_epi16(addu, zero);\
        const __m128i add1 = _mm_unpackhi_epi16(addu, zero);\
        const __m128i v01_0a = SLL32(add0, a);\
        const __m128i v23_0a = SLL32(add1, a);

#define EXIT_ME\
        m = CountTrailingZeroBits(ui32(~((m << 4) | _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(c, r0))))));\
        _mm_storeu_si128((__m128i *)(hitGroup.Docids + 0), r0);\
        _mm_storeu_si128((__m128i *)(hitGroup.Docids + 4), r1);\
        hitGroup.SameDoc = false;\
        hitGroup.Ptr = m + 1;\
        hitGroup.End = count < 0 ? count + 8 : 8;\
        hitGroup.Count = count;\
        hitGroup.OldPostings = old;\
        codedIn = (const char *)coded;\
        current = SUPERLONG(hitGroup.Docids[m]) << DOC_LEVEL_Shift;\
        return true;

OPTIMIZED RELEASE_INLINE
int DecodeHiSIMD(int code, const ui8 *coded, const __m128i &docId, __m128i &r0, __m128i &r1) {
    const __m128i zero = _mm_setzero_si128();
    if (Y_UNLIKELY(code < 65)) {
        OP_0_SETUP;
        r0 = _mm_add_epi32(docId, _mm_unpacklo_epi16(v0, zero));
        r1 = _mm_add_epi32(docId, _mm_unpackhi_epi16(v0, zero));
        return 1;
    } else {
        int a = Doc0::itable[code];
        if (Y_LIKELY(a < 12)) {
            OP_1_SETUP;
            r0 = _mm_add_epi32(docId, _mm_unpacklo_epi16(v0, zero));
            r1 = _mm_add_epi32(docId, _mm_unpackhi_epi16(v0, zero));
        } else {
            OP_2_SETUP;
            r0 = _mm_add_epi32(s0, v01_0a);
            r1 = _mm_add_epi32(s1, v23_0a);
        }
        return a + 1;
    }
}

OPTIMIZED
const char *DecompressAll(THitGroup &hitGroup, const char *codedIn) {
    const ui8 *coded = (const ui8 *)codedIn;
    const int mask = coded[0];

    __m128i r0, r1;
    if (mask & 0x80) {
        coded = DecodeLoSIMDTemplate<true>(mask, coded, r0, r1);
        _mm_storeu_si128((__m128i *)(hitGroup.Postings + 0), r0);
        _mm_storeu_si128((__m128i *)(hitGroup.Postings + 4), r1);

        const __m128i docId = _mm_set1_epi32(hitGroup.Docids[7]);
        coded += DecodeHiSIMD(coded[0], coded + 1, docId, r0, r1);
        _mm_storeu_si128((__m128i *)(hitGroup.Docids + 0), r0);
        _mm_storeu_si128((__m128i *)(hitGroup.Docids + 4), r1);

        hitGroup.SameDoc = false;
    } else {
        coded = DecodeLoSIMDTemplate<false>(mask, coded, r0, r1);
        _mm_storeu_si128((__m128i *)(hitGroup.Postings + 0), r0);
        _mm_storeu_si128((__m128i *)(hitGroup.Postings + 4), r1);

        hitGroup.SameDoc = true;
    }

    hitGroup.Ptr = 0;
    hitGroup.OldPostings = nullptr;

    return (const char *)coded;
}

OPTIMIZED
bool DecompressSkip(SUPERLONG &current, THitGroup &hitGroup, const char *&codedIn, i32 docSkip) {
    const ui8 *coded = (const ui8 *)codedIn;
    int count = hitGroup.Count;
    const __m128i zero = _mm_setzero_si128();
    const __m128i c = _mm_set1_epi32(docSkip);
    __m128i docId = _mm_set1_epi32(hitGroup.Docids[7]);
    while (count > 0) {
        const int mask = coded[0];
        count -= 8;
        if (mask & 0x80) {
            __m128i r0, r1;
            const ui8 *old = coded;
            coded = Skip<true>(mask, coded);
            int code = coded[0];
            ++coded;
            if (Y_UNLIKELY(code < 65)) {
                OP_0_SETUP;
                r1 = _mm_add_epi32(docId, _mm_unpackhi_epi16(v0, zero));
                int m = _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(c, r1)));
                if (m != 0xf) {
                    r0 = _mm_add_epi32(docId, _mm_unpacklo_epi16(v0, zero));
                    EXIT_ME;
                }
            } else {
                int a = Doc0::itable[code];
                if (Y_LIKELY(a < 12)) {
                    OP_1_SETUP;
                    coded += a;
                    r1 = _mm_add_epi32(docId, _mm_unpackhi_epi16(v0, zero));
                    int m = _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(c, r1)));
                    if (m != 0xf) {
                        r0 = _mm_add_epi32(docId, _mm_unpacklo_epi16(v0, zero));
                        EXIT_ME;
                    }
                } else {
                    OP_2_SETUP;
                    coded += a;
                    r1 = _mm_add_epi32(s1, v23_0a);
                    int m = _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(c, r1)));
                    if (m != 0xf) {
                        r0 = _mm_add_epi32(s0, v01_0a);
                        EXIT_ME;
                    }
                }
            }
            docId = _mm_shuffle_epi32(r1, _MM_SHUFFLE(3, 3, 3, 3));
        } else {
            coded = Skip<false>(mask, coded);
        }
    }
    codedIn = (const char *)coded;
    hitGroup.Ptr = 0;
    hitGroup.End = 0;
    return false;
}

OPTIMIZED
void DecodePostings(THitGroup &hitGroup) {
    __m128i r0, r1;
    DecodeLoSIMDTemplate<true>(hitGroup.OldPostings[0], hitGroup.OldPostings, r0, r1);
    _mm_storeu_si128((__m128i *)(hitGroup.Postings + 0), r0);
    _mm_storeu_si128((__m128i *)(hitGroup.Postings + 4), r1);
}

__m128i RELEASE_INLINE PackU32(const __m128i &v0, const __m128i &v1) {
    const __m128i lo = _mm_shuffle_epi32(v0, _MM_SHUFFLE(2, 0, 2, 0));
    const __m128i hi = _mm_shuffle_epi32(v1, _MM_SHUFFLE(2, 0, 2, 0));
    return _mm_unpacklo_epi64(lo, hi);
}


int RELEASE_INLINE Transform(ui8 *data, __m128i &doc, __m128i &phrase, __m128i &word, __m128i &form) {
    const __m128i s0 = _mm_loadu_si128((const __m128i *)(data - 8));
    const __m128i s1 = _mm_loadu_si128((const __m128i *)(data + 0));
    const __m128i s2 = _mm_loadu_si128((const __m128i *)(data + 8));
    const __m128i s3 = _mm_loadu_si128((const __m128i *)(data + 16));

    const __m128i d0 = _mm_sub_epi64(_mm_srli_epi64(s1, 27), _mm_srli_epi64(s0, 27));
    const __m128i d1 = _mm_sub_epi64(_mm_srli_epi64(s3, 27), _mm_srli_epi64(s2, 27));
    doc = PackU32(d0, d1);

    const __m128i p0 = PackU32(s1, s3);
    const __m128i p1 = PackU32(s0, s2);

    const __m128i phrase0 = _mm_and_si128(masks32[15], _mm_srli_epi32(p0, 12));
    const __m128i phrase1 = _mm_and_si128(masks32[15], _mm_srli_epi32(p1, 12));
    word = _mm_and_si128(masks32[6], _mm_srli_epi32(p0, 6));
    form = _mm_and_si128(masks32[6], p0);

    const __m128i dzero = _mm_cmpeq_epi32(doc, _mm_setzero_si128());
    phrase = _mm_sub_epi32(phrase0, _mm_and_si128(phrase1, dzero));

    return _mm_movemask_epi8(dzero);
}


OPTIMIZED
ui8 *Compress(const SUPERLONG *from, ui8 *coded, ui8) {
    __m128i d0, d1;
    __m128i p0, p1;
    __m128i w0, w1;
    __m128i f0, f1;

    int m0 = Transform(((ui8 *)from) +  0, d0, p0, w0, f0);
    int m1 = Transform(((ui8 *)from) + 32, d1, p1, w1, f1);

    bool zero = (m0 + m1) == (0xffff * 2);

    if (zero) {
        size_t shift = formRelevCompressor1.GetCode<false>(f0, f1, coded[0]);
        __m128i comp0 = f0;
        __m128i comp1 = f1;

        size_t shift1x = wordCompressor1.GetCode<false>(w0, w1, coded[1]);
        comp0 = _mm_or_si128(comp0, SLL32(w0, shift));
        comp1 = _mm_or_si128(comp1, SLL32(w1, shift));
        shift += shift1x;

        size_t shift2x = phraseCompressor1.GetCode<false>(p0, p1, coded[2]);
        comp0 = _mm_or_si128(comp0, SLL32(p0, shift));
        comp1 = _mm_or_si128(comp1, SLL32(p1, shift));
        shift += shift2x;

        coded = Flush(comp0, comp1, shift, coded + 3);

    } else {
        size_t shift = formRelevCompressor0.GetCode<false>(f0, f1, coded[0]);
        __m128i comp0 = f0;
        __m128i comp1 = f1;
        coded[0] += 0x80;

        size_t shift1x = wordCompressor0.GetCode<false>(w0, w1, coded[1]);
        comp0 = _mm_or_si128(comp0, SLL32(w0, shift));
        comp1 = _mm_or_si128(comp1, SLL32(w1, shift));
        shift += shift1x;

        size_t shift2x = phraseCompressor0.GetCode<false>(p0, p1, coded[2]);
        comp0 = _mm_or_si128(comp0, SLL32(p0, shift));
        comp1 = _mm_or_si128(comp1, SLL32(p1, shift));
        shift += shift2x;

        coded = Flush(comp0, comp1, shift, coded + 3);

        shift = docCompressor0.GetCode<true>(d0, d1, coded[0]);
        coded = Flush(d0, d1, shift, coded + 1);
    }
    return coded;
}
