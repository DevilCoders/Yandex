#pragma once

#include <library/cpp/sse/sse.h>

#include "hitformat.h"
#include "hitread.h"
#include "full_pos.h"
#include "longs.h"

#include <library/cpp/wordpos/wordpos.h>

#include <kernel/keyinv/hitlist/inv_code/bit_code.h>

#include <util/generic/yexception.h>
#include <util/system/defaults.h>

// has to be in longs.h ?
#define MAX_PACKED_WORDPOS_SIZE 16 // @todo for HIT_FMT_BLK8 this size must be increased

class CHitGroupCoder {
    SUPERLONG Data[9];
    size_t Ptr;
public:
    CHitGroupCoder()
        : Ptr(1)
    {
        Data[0] = START_POINT;
    }

    Y_FORCE_INLINE void SetCurrent(const SUPERLONG &l) {
        Data[0] = l;
    }
    //! @return true if Flush() must be called immediately after
    bool Output(SUPERLONG l);
    //! @return number of written bytes
    int Flush(char *dst);
    void Reset() {
        Ptr = 1;
        Data[0] = START_POINT;
    }
    const SUPERLONG* GetData() const {
        return Data;
    }
    size_t GetPtr() const {
        return Ptr;
    }
};
//////////////////////////////////////////////////////////////////////////
class CHitCoder {
    CHitGroupCoder GroupCoder;
    SUPERLONG Current;
    EHitFormat HitFormat;
public:
    CHitCoder(EHitFormat format = HIT_FMT_V2) {
        Reset();
        SetFormat(format);
    }
    int DumpJunk(char *dst) {
        if (HitFormat == HIT_FMT_BLK8) {
            for (size_t i = 0; i < 7; ++i)
                ((ui8 *)dst)[i] = 0xff;
            return 7;
        }
        return 0;
    }
private:
    int DumpHeader(char* dst) {
        if (HitFormat == HIT_FMT_BLK8) {
            const SUPERLONG* data = GroupCoder.GetData();
            assert(GroupCoder.GetPtr() > 1 && data[0] == START_POINT);
            ui8 *ptr = (ui8 *)dst;
            SUPERLONG doc = data[1] >> DOC_LEVEL_Shift;
            ptr[0] = (doc >> 0) & 0xff;
            ptr[1] = (doc >> 8) & 0xff;
            ptr[2] = (doc >> 16) & 0xff;
            SetCurrent((doc & 0xffffff) << DOC_LEVEL_Shift);
            return 3;
        }
        return 0;
    }
public:
    void Reset() {
        Current = START_POINT;
        GroupCoder.Reset();
    }
    EHitFormat GetFormat() const {
        return HitFormat;
    }
    void SetFormat(EHitFormat formatEx) {
        HitFormat = formatEx;
    }
    Y_FORCE_INLINE SUPERLONG GetCurrent() const {
        return Current;
    }
    Y_FORCE_INLINE void SetCurrent(const SUPERLONG &l) {
         Current = l;
         GroupCoder.SetCurrent(l);
    }
    int Output(SUPERLONG l, char *dst);
    int Flush(char* dst) {
        return GroupCoder.Flush(dst);
    }
    int Finish(char* dst);
    int FinishNoJunk(char* dst, bool& dj);
    void Swap(CHitCoder& rhs);
};

//////////////////////////////////////////////////////////////////////////

class CHitDecoder {

protected:
    EHitFormat HitFormat;
    SUPERLONG Current;
    SUPERLONG CurrentInternal;

private:
    inline void ReadNext(const char *&data) {
#ifndef NDEBUG
        SUPERLONG OldCurrent = CurrentInternal;
#endif
#if (defined(_unix_) && defined(_i386_)) && !defined(_cygwin_) && !defined(_darwin_)
        ASMi386_UNPACK_ADD_64(data, CurrentInternal);
#elif defined(_MSC_VER) && defined(_i386_)
        _asm {//ecx is 'this'
            mov ecx,this _asm mov esi, data //prologue
            mov esi, [esi]
            movzx eax, byte ptr [esi] _asm inc esi
            xor edx, edx // touch edx here to inform compiler edx is modified
            call i386unp_bufba[eax*4] //unlike GNU C, ecx' role there is esi
            add dword ptr ([ecx]this.CurrentInternal), eax
            adc dword ptr ([ecx+4]this.CurrentInternal), edx
            mov eax, data
            mov [eax], esi
        }
#else
        SUPERLONG diff;
        int ret = 0; // not used actually
        UNPACK_64(diff, data, mem_traits, ret);
        ++ret;
        CurrentInternal += diff;
#endif
        assert(CurrentInternal >= OldCurrent);
    }

public:
    CHitDecoder(EHitFormat format = HIT_FMT_UNKNOWN)
        : HitFormat(format) {
        Reset();
    }
    void SetFormat(EHitFormat format) {
        HitFormat = format;
    }
    EHitFormat GetFormat() const {
        return HitFormat;
    }
    void SetCurrent(SUPERLONG l);
    void SetCurrentNative(SUPERLONG l);
    void Next(const char *&data);
    bool SkipTo(const char *&Cur0, const char *Upper, SUPERLONG to);
    void Reset() {
        Current = START_POINT;
        CurrentInternal = START_POINT;
    }
    Y_FORCE_INLINE SUPERLONG GetCurrent() const {
        return Current;
    }
};

//////////////////////////////////////////////////////////////////////////
class CHitDecoder2 {
protected:
    SUPERLONG Current;
    EHitFormat HitFormat;

public:
    CHitDecoder2()
        : HitFormat(0)
    {
        Reset();
    }

    Y_FORCE_INLINE void SetCurrent(SUPERLONG l) {
        Current = l;
    }

    Y_FORCE_INLINE void SetCurrentNative(SUPERLONG l) {
        Current = l;
    }

    void Next(const char*& data);
    bool SkipTo(const char*& cur0, const char* upper, SUPERLONG to);

    EHitFormat GetFormat() {
        return HitFormat;
    }

    void SetFormat(EHitFormat format) {
        if (HIT_FMT_V2 != format)
            ythrow yexception() << "wrong index format";
        HitFormat = format;
    }

    void Reset() {
        Current = START_POINT;
    }

    Y_FORCE_INLINE SUPERLONG GetCurrent() const {
        return Current;
    }
};
//////////////////////////////////////////////////////////////////////////
template<typename T, bool Strict, EHitFormat TerminalFormat>
class DecoderFallBack : public T, public THitGroup {

public:

    void Reset() {
        T::Reset();
        THitGroup::Reset();
    }

    static bool CanFold() {
        return Strict;
    }

    DecoderFallBack() {
    }

    template<bool SAMEDOC>
    Y_FORCE_INLINE ui32 NextPosting(i32 docid, ui32 posting, int ptr) {
        if (SAMEDOC) {
            return (posting & BREAK_LEVEL_Mask) + Postings[ptr];
        } else {
            return (docid == Docids[ptr]) ? (posting & BREAK_LEVEL_Mask) + Postings[ptr] : POSTING_ERR;
        }
    }

    Y_FORCE_INLINE void NextElem(int ptr) {
        const SUPERLONG nextDoc = SameDoc ? this->Current : SUPERLONG(Docids[ptr]) << DOC_LEVEL_Shift;
        this->Current = (Max(this->Current, nextDoc) & DOCBREAK_LEVEL_Mask) + Postings[ptr];
    }

    Y_FORCE_INLINE void FetchPosting(ui32 posting, TFullPosition*& positions, const ui8* formPriorities, const ui64* formMasks, ui64& globalMask) {
        const i32 form = posting & NFORM_LEVEL_Mask;
        const ui8 formPrior = formPriorities[form];
        if (formPrior != NUM_FORM_CLASSES) {
            const i32 posBeg = posting & BREAKWORDRELEV_LEVEL_Mask;
            i32 posEnd = posBeg | formPrior;
            if (Y_UNLIKELY(positions[0].Beg >= posBeg)) {
                posEnd = Min(positions[0].End, posEnd);
            } else {
                ++positions;
            }
            positions[0].Beg = posBeg;
            positions[0].End = posEnd;

            if (formPrior != EQUAL_BY_SYNSET) {
                globalMask |= formMasks[form];
            }
        }
    }

    template<bool SAMEDOC>
    Y_FORCE_INLINE bool FetchChunkInt(TFullPosition*& positions, SUPERLONG bound, const ui8* formPriorities, const ui64* formMasks, ui64& globalMask) {
        assert(Strict && TerminalFormat == this->HitFormat);
        int ptr = Ptr;
        const int end = End;
        TFullPosition *positionsLocal = positions;
        ui64 mask = globalMask;

        const SUPERLONG current = this->Current;
        const i32 curDocid = current >> DOC_LEVEL_Shift;
        ui32 curPosting = current & POSTING_MAX;
        const ui32 postingBound = bound & POSTING_MAX;
        do {
            FetchPosting(curPosting, positionsLocal, formPriorities, formMasks, mask);
            if (Y_LIKELY(ptr < end)) {
                curPosting = NextPosting<SAMEDOC>(curDocid, curPosting, ptr);
                ++ptr;
            } else {
                Ptr = ptr;
                this->Current = (current & DOC_LEVEL_Mask) | curPosting;
                globalMask = mask;
                positions = positionsLocal;
                return false;
            }
        } while (curPosting <= postingBound);
        Ptr = ptr;
        if (SAMEDOC) {
            this->Current = (current & DOC_LEVEL_Mask) | curPosting;
        } else {
            this->Current = (curPosting <= POSTING_MAX) ?
                (current & DOC_LEVEL_Mask) | curPosting :
                (SUPERLONG(Docids[Ptr - 1]) << DOC_LEVEL_Shift) | Postings[Ptr - 1];
        }
        globalMask = mask;
        positions = positionsLocal;
        return true;
    }

    Y_FORCE_INLINE bool FetchChunk(TFullPosition*& positions, SUPERLONG bound, const ui8* formPriorities, const ui64* formMasks, ui64& globalMask) {
        return Y_LIKELY(SameDoc) ?
            FetchChunkInt<true>(positions, bound, formPriorities, formMasks, globalMask) :
            FetchChunkInt<false>(positions, bound, formPriorities, formMasks, globalMask);
    }

    Y_FORCE_INLINE bool Fetch() {
        if (Y_LIKELY(Ptr < End)) {
            NextElem(Ptr);
            ++Ptr;
            return true;
        }
        return false;
    }

    void FallBackDecode(const char *&data, ui32 size) {
        if (Strict || TerminalFormat == this->HitFormat) {
            const SUPERLONG oldCurrent = this->Current;
            i32  oldDoc = i32(oldCurrent >> DOC_LEVEL_Shift);
            ui32 oldPhrase = ui32(oldCurrent & BREAK_LEVEL_Mask);
            this->HitFormat = HIT_FMT_V2;
            for (size_t i = 0; i < size; ++i) {
                T::Next(data);
                i32 newDoc = i32(this->Current >> DOC_LEVEL_Shift);
                ui32 newPosting = ui32(this->Current & POSTING_MAX);
                Docids[i] = newDoc;
                if (Y_LIKELY(newDoc == oldDoc)) {
                    Postings[i] = newPosting - oldPhrase;
                } else {
                    Postings[i] = newPosting;
                    oldDoc = newDoc;
                }
                oldPhrase = newPosting & BREAK_LEVEL_Mask;
            }
            Docids[7] = Docids[size - 1];
            SameDoc = (i32(oldCurrent >> DOC_LEVEL_Shift) == Docids[7]);
            Ptr = 0;
            End = size;
            Count = -1;
            this->Current = oldCurrent;
            this->HitFormat = TerminalFormat;
        }
    }

    Y_FORCE_INLINE void ReadHeader(const char *&data) {
        if (Strict || TerminalFormat == this->HitFormat) {
            assert(TerminalFormat == this->HitFormat);
            const ui8 *ptr = (const ui8 *)data;
            SUPERLONG doc = 0;
            doc += SUPERLONG(ptr[0]) << 0;
            doc += SUPERLONG(ptr[1]) << 8;
            doc += SUPERLONG(ptr[2]) << 16;
            doc = (doc << DOC_LEVEL_Shift);
            data += 3;
            SetCurrentNative(doc);
            this->HitFormat = HIT_FMT_V2;
            T::SetCurrentNative(doc);
            this->HitFormat = TerminalFormat;
        }
    }

    Y_FORCE_INLINE void Next(const char *&data) {
        if (Strict || TerminalFormat == this->HitFormat) {
            assert(TerminalFormat == this->HitFormat);
            data = DecompressAll(*this, data);
            Ptr = 1;
            End = Count < 8 ? Count : 8;
            Count -= 8;
            NextElem(0);
            return;
        } else {
             T::Next(data);
        }
    }

    Y_FORCE_INLINE void Next(const char *&data, const char *page) {
        if (Strict || TerminalFormat == this->HitFormat) {
            assert(TerminalFormat == this->HitFormat);
            if (Count < 64 && page < data + 128) {
                char temp[128 + 8];
                const char *tempData = temp;
                memcpy(temp, data, page - data);
                Next(tempData);
                data += (tempData - temp);
            } else {
                Next(data);
            }
        } else {
            Next(data);
        }
    }

    Y_FORCE_INLINE bool SkipTo(const char *&cur0, const char* upper, SUPERLONG to) {
        if (Strict || TerminalFormat == this->HitFormat) {
            assert(TerminalFormat == this->HitFormat);

            if (Y_UNLIKELY(this->Current >= to)) {
                return true;
            }

            // When called to advance to the next document.
            if (Y_UNLIKELY((this->Current ^ to) & DOC_LEVEL_Mask)) {
                const i32 toDocid = to >> DOC_LEVEL_Shift;
                if (!SmallSkipToDoc(toDocid) && !DecompressSkip(this->Current, *this, cur0, toDocid)) {
                    return false;
                }
                Touch();
                if (Y_LIKELY(this->Current >= to)) {
                    return true;
                }
            }
            assert((this->Current & DOC_LEVEL_Mask) == (to & DOC_LEVEL_Mask));

            const SUPERLONG curDoc = this->Current & DOC_LEVEL_Mask;
            const i32 curDocid = this->Current >> DOC_LEVEL_Shift;
            ui32 curPosting = this->Current & POSTING_MAX;
            const ui32 toPosting = to & POSTING_MAX;
            if (SameDoc || (curDocid == Docids[7])) {
                do {
                    for (int i = Ptr; i < End; ++i) {
                        curPosting = NextPosting<true>(curDocid, curPosting, i);
                        if (Y_UNLIKELY(curPosting >= toPosting)) {
                            Ptr = i + 1;
                            this->Current = curDoc | curPosting;
                            return true;
                        }
                    }

                    if (Y_UNLIKELY(cur0 >= upper)) {
                        this->Current = curDoc | curPosting;
                        return false;
                    }
                    End = Count < 8 ? Count : 8;
                    Count -= 8;
                    cur0 = DecompressAll(*this, cur0);
                } while (SameDoc);
            }
            for (int i = Ptr; i < End; ++i) {
                curPosting = NextPosting<false>(curDocid, curPosting, i);
                if (Y_UNLIKELY(curPosting >= toPosting)) {
                    Ptr = i + 1;
                    this->Current = (curPosting <= POSTING_MAX) ? curDoc | curPosting : (SUPERLONG(Docids[i]) << DOC_LEVEL_Shift) | Postings[i];
                    return true;
                }
            }
            assert(0);
            return false;
        } else {
            return T::SkipTo(cur0, upper, to);
        }
    }

    Y_FORCE_INLINE void Touch() {
        if (OldPostings) {
            DecodePostings(*this);
            OldPostings = nullptr;
            this->Current |= Postings[Ptr - 1];
        }
    }

    Y_FORCE_INLINE bool SmallSkipToDoc(i32 docid) {
        if (!SameDoc && (docid <= Docids[7]) && (Ptr < End)) {
#if !defined(_MSC_VER)
            // It is not strictly proven that SSE code is faster here,
            // see benchmark at junk/mvel/tests/mvel-msan-hitcoders
            // but profiling still shows better performance for SSE version.
#define CMP_4DOCIDS(d4, cur) ( \
            _mm_movemask_ps( \
                _mm_castsi128_ps( \
                    _mm_cmplt_epi32( \
                        _mm_loadu_si128((const __m128i *)(d4)), cur \
                    ) \
                ) \
            ) \
)
            const __m128i docids = _mm_set1_epi32(docid);
            const int i = CountTrailingZeroBits(ui32(
                ~(CMP_4DOCIDS(Docids + 0, docids) | CMP_4DOCIDS(Docids + 4, docids) << 4)
            ));
#undef CMP_4DOCIDS
            Y_ASSERT(i >= Ptr && i < End);
            Ptr = i + 1;
            this->Current = SUPERLONG(Docids[i]) << DOC_LEVEL_Shift;
            this->Current |= OldPostings ? 0 : Postings[i];
            return true;
#else
            for (int i = Ptr; i < End; ++i) {
                if (Y_UNLIKELY(Docids[i] >= docid)) {
                    Ptr = i + 1;
                    this->Current = SUPERLONG(Docids[i]) << DOC_LEVEL_Shift;
                    this->Current |= OldPostings ? 0 : Postings[i];
                    return true;
                }
            }
#endif
        }
        return false;
    }

    Y_FORCE_INLINE bool SkipToDoc(const char *&cur0, const char* upper, SUPERLONG to) {
        if (Strict || TerminalFormat == this->HitFormat) {
            assert(TerminalFormat == this->HitFormat);
            return DecompressSkip(this->Current, *this, cur0, i32(to >> DOC_LEVEL_Shift));
        } else {
            return T::SkipTo(cur0, upper, to);
        }
    }

    Y_FORCE_INLINE EHitFormat GetFormat() {
        return this->HitFormat;
    }

    Y_FORCE_INLINE void SetCurrent(SUPERLONG l) {
        if (Strict || TerminalFormat == this->HitFormat) {
            assert(TerminalFormat == this->HitFormat);
            Docids[7] = i32(l >> DOC_LEVEL_Shift);
            Ptr = End = 0;
            this->Current = l;
        }
        else T::SetCurrent(l);
    }

    Y_FORCE_INLINE void SetCurrentNative(SUPERLONG l) {
        if (Strict || TerminalFormat == this->HitFormat) {
            assert(TerminalFormat == this->HitFormat);
            Docids[7] = i32(l >> DOC_LEVEL_Shift);
            Ptr = End = 0;
            this->Current = l;
        }
        else T::SetCurrentNative(l);
    }

    Y_FORCE_INLINE void SetFormat(EHitFormat formatEx) {
        EHitFormat format = formatEx;
        if (Strict || TerminalFormat == format) {
            this->HitFormat = format;
        }
        else T::SetFormat(format);
    }

    static Y_FORCE_INLINE bool NotCompressed() {
        return Strict;
    }

    static Y_FORCE_INLINE ui32 GetPerstSize(const THitsForRead &hitsForRead) {
        if (Strict)
            return 12;
        else
            return hitsForRead.GetSubIndexInfo().nPerstSize;
    }

    Y_FORCE_INLINE i64 GetHitsLength(const THitsForRead &hitsForRead) {
        if (Strict || TerminalFormat == this->HitFormat)
            return i64(hitsForRead.GetLength()) - 7;
        else
            return hitsForRead.GetLength();
    }

    static Y_FORCE_INLINE ui32 GetMinNeedSize(const THitsForRead &hitsForRead) {
        if (Strict)
            return 128;
        else
            return hitsForRead.GetSubIndexInfo().nMinNeedSize;
    }

    static Y_FORCE_INLINE ui32 GetSubIndexStep(const THitsForRead &hitsForRead) {
        if (Strict)
            return 64;
        else
            return hitsForRead.GetSubIndexInfo().nSubIndexStep;
    }
};


template<EHitFormat TerminalFormat>
Y_FORCE_INLINE ui32 GetHitsLength(ui32 length, i64 count, EHitFormat format) {
    if (TerminalFormat == format && count > 7)
        return length - 7;
    else
        return length;
}
//////////////////////////////////////////////////////////////////////////
class TSkip {
public:
    template<class T>
    static Y_FORCE_INLINE bool SkipTo(T &decoder, const char *&cur0, const char* upper, SUPERLONG to) {
        return decoder.SkipTo(cur0, upper, to);
    }

    template<class T>
    static Y_FORCE_INLINE bool SmallSkipTo(T &, SUPERLONG ) {
        return false;
    }
};
