#pragma once

#include <util/system/defaults.h>

#include "hitread.h"
#include "hits_coders.h"

extern void BreakFunc(int size);

//! a forward-only iterator of key positions
template<typename TDecoder = DecoderFallBack<CHitDecoder, false, HIT_FMT_BLK8 > >
class TPosIterator
{
public:
    typedef TPosIterator* ptr;
    typedef SUPERLONG value_type;
public:

    static bool CanFold() {
        return TDecoder::CanFold();
    }

    static SUPERLONG MaxValue() {
        return (1ULL << 63) - 1;
    }

    TPosIterator()
      : Cur0(nullptr)
      , EofFlag(true)
      , CurPerst(&YxPerst::NullSubIndex)
      , CurPerstData(nullptr)
      , CurPerstDataEnd(nullptr)
    {
    }

    TPosIterator(const IKeysAndPositions& index, const char* key, READ_HITS_TYPE type, TRequestContext *cache = nullptr);

    ui64 GetCurrentPerst() const {
        if (HitsForRead.GetFirstPerst() == &YxPerst::NullSubIndex || CurPerst == &YxPerst::NullSubIndex)
            return 0;
        return ((char*)CurPerst - (char*)HitsForRead.GetFirstPerst()) / TDecoder::GetPerstSize(HitsForRead);
    }

    ui64 GetLastPerst() const {
        if (HitsForRead.GetFirstPerst() == &YxPerst::NullSubIndex || CurPerst == &YxPerst::NullSubIndex)
            return 0;
        return ((char*)HitsForRead.GetLastPerst() - (char*)HitsForRead.GetFirstPerst()) /
            TDecoder::GetPerstSize(HitsForRead);
    }

    bool Init(const IKeysAndPositions& index, const char* key, READ_HITS_TYPE type, TRequestContext *cache = nullptr);
    void Init(const IKeysAndPositions& index, i64 offset, ui32 length, i64 count, READ_HITS_TYPE type);
    void Init(const TIndexInfo& si, TBlob& parent, i64 parent_offset, i64 offset, ui32 length, i64 count);
    void Init(const THitsForRead& other);

    void SaveIndexAccess(IOutputStream* rh) const {
        HitsForRead.SaveIndexAccess(rh);
    }

    void RestoreIndexAccess(IInputStream* rh, const IKeysAndPositions& index, READ_HITS_TYPE type = RH_DEFAULT) {
        HitsForRead.RestoreIndexAccess(rh, index, type);
        Initialize();
    }

    bool Valid() const {
        return !EofFlag;
    }

    size_t GetCount() const {
        return HitsForRead.GetCount();
    }

    SUPERLONG Current() const {
        //assert(!EofFlag);
        return Decoder.GetCurrent();
    }

    SUPERLONG operator *() {
        //assert(!EofFlag);
        return Current();
    }

    ui32 Doc() const {
        assert(!EofFlag);
        return TWordPosition::Doc(Current());
    }

    ui32 Break() const {
        assert(!EofFlag);
        return TWordPosition::Break(Current());
    }

    ui32 Word() const {
        assert(!EofFlag);
        return TWordPosition::Word(Current());
    }

    ui32 RelevLevel() const {
        assert(!EofFlag);
        return (ui32)TWordPosition::GetRelevLevel(Current());
    }

    ui32 DocLength() const {
        assert(!EofFlag);
        return TWordPosition::DocLength(Current());
    }

    TPosIterator& operator ++ () {
        Next();
        return *this;
    }

    void Restart()
    {
        InternalRestart();
    }

     void Y_FORCE_INLINE InternalFetch() {
        if (Y_LIKELY(Cur0 < CurPerstDataEnd)) {
            Decoder.Next(Cur0);
        } else if (CurPerst->Sum < YxPerst::NullSubIndex.Sum) { //В конце всегда лежит NULL_PERST
            ChangeCurrentPerst<false>(Advance(CurPerst, 1, TDecoder::GetPerstSize(HitsForRead)));
            if (CurPerstDataEnd != CurPerstData) {
                Decoder.Next(Cur0);
            } else {
                SetEndOfHits();
            }
        } else {
            SetEndOfHits();
        }
    }

    //! moves to the next position
    void Y_FORCE_INLINE Next() {
        assert(!EofFlag);
        if (!Decoder.Fetch()) {
            InternalFetch();
        }
    }

    //! reads chunk of data
    void Y_FORCE_INLINE NextChunk(TFullPosition *&positions, SUPERLONG bound, const ui8* formPriorities, const ui64* formMasks, ui64 &mask) {
        assert(!EofFlag);
        if (!Decoder.FetchChunk(positions, bound, formPriorities, formMasks, mask)) {
            InternalFetch();
        }
    }

    ptr TopIter() { // sorry, only for templates
       assert(Valid());
       return this;
    }

    void SkipTo(ui32 doc, ui32 brk, ui32 word) {
        return SkipTo(TWordPosition(doc, brk, word).SuperLong());
    }

    //! skips to the specified position that should be greater or equal to the current position
    //! @param to       a position to skip to
    //! @note when this member function is called iterator must be valid
    //!       if position is less than the current position (moving back) iterator remains at the current position
    //!       if position is not found iterator stops at the first position that is greater
    //!       if position is greater than the last position iterator gets invalid
    void SkipTo(SUPERLONG to) {
        return IntSkipTo<TSkip>(to);
    }

    template<typename TSmartSkip>
    void SkipTo(SUPERLONG to, const TSmartSkip &) {
        return IntSkipTo<TSmartSkip>(to);
    }

    void Y_FORCE_INLINE Touch() {
        Decoder.Touch();
    }

    //! skips to the specified position that should be greater or equal to the current position and counts skips
    //! @param to       a position to skip to
    //! @param count    accumulative skip counter whose value is counted from the first position
    //! @note this member function is used in zone search
    //! @attention the value of the @c count variable must be stored between calls to this function
    void SkipAndCount(SUPERLONG to, i64 &count);

    //! skips from current position with index @c countOld to new position with index @c countNew
    //! @param countOld     the index of the current position
    //! @param countNew     the index of the new position to which this function moves iterator
    //! @note this member function is used in zone search
    //! @attention the value of the @c countOld variable must be stored between calls to this function
    void SkipToCount(const i64 countOld, const i64 countNew);

    friend bool operator < (const TPosIterator &lh, const TPosIterator &rh) {
        return !lh.EofFlag && (rh.EofFlag || lh.Current() < rh.Current());
    }
    friend bool operator < (const TPosIterator &lh, const SUPERLONG &rh) {
        return !lh.EofFlag && lh.Current() < rh;
    }
    friend bool operator == (const TPosIterator &lh, const TPosIterator &rh) {
        return lh.EofFlag == rh.EofFlag && (lh.EofFlag || lh.Cur0 == rh.Cur0);
    }
    friend bool operator != (const TPosIterator &lh, const TPosIterator &rh) {
        return !(lh == rh);
    }

private:
    const char *GetCur() {
        return Cur0;
    }
    const YxPerst *GetPerst() {
        return CurPerst;
    }

private:

    template<typename DocSkip>
    void Y_FORCE_INLINE IntSkipTo(SUPERLONG to)
    {
        assert(!EofFlag);

#ifndef NDEBUG
        SUPERLONG cur = Current();
#endif
        const YxPerst *newPerst = CurPerst;
        if (newPerst->Sum < to) {
            int perstSize = TDecoder::GetPerstSize(HitsForRead);

            newPerst = Advance(newPerst, perstSize);
            if (newPerst->Sum < to) {

                const YxPerst *end = HitsForRead.GetLastPerst();
                int add = perstSize;
                while (1) {
                    const YxPerst *step = Advance(newPerst, add);
                    if (step > end) {
                        add = add >> 1;
                    } else {
                        if (step->Sum < to) {
                            newPerst = step;
                            add = add << 1;
                        } else {
                            break;
                        }
                    }
                }

                while (1) {
                    const YxPerst *step = Advance(newPerst, add);
                    newPerst = step->Sum < to ? step : newPerst;
                    if (add == perstSize)
                        break;
                    add = add >> 1;
                }
                newPerst = Advance(newPerst, perstSize);

            }
            ChangeCurrentPerst<false>(newPerst);
            assert(Decoder.GetCurrent() < to);
        } else {
            if (DocSkip::SmallSkipTo(Decoder, to))
                return;
        }

        if (!DocSkip::SkipTo(Decoder, Cur0, CurPerstDataEnd, to)) {
            SetEndOfHits();
        }

        assert(EofFlag || Current() >= cur);
    }

    template< bool First>
    void Y_FORCE_INLINE ChangeCurrentPerst(const YxPerst *newPerst) {

        const YxPerst *p = First ? nullptr : Advance(newPerst, -(int)TDecoder::GetPerstSize(HitsForRead));
        size_t lowBound = First ? 0 : p->Off;
        Decoder.SetCurrentNative(First ? START_POINT : p->Sum);
        size_t highBound;

        if (newPerst->Sum == YxPerst::NullSubIndex.Sum) {
            highBound = Decoder.GetHitsLength(HitsForRead);
            if (HitsForRead.GetCount() <= TDecoder::GetMinNeedSize(HitsForRead) || newPerst == &YxPerst::NullSubIndex)
                Decoder.SetSize(HitsForRead.GetCount());
            else
                Decoder.SetSize(HitsForRead.GetCount() % TDecoder::GetSubIndexStep(HitsForRead));
        } else {
            highBound = newPerst->Off;
            Decoder.SetSize(TDecoder::GetSubIndexStep(HitsForRead));
        }

        const TBlob &perstBlob = HitsForRead.GetData();
        const char *data = perstBlob.AsCharPtr();
        {
            CurPerstData = data + lowBound;
            if ((size_t)data + highBound < highBound) {
                CurPerstDataEnd = data - (Max<size_t>() - highBound);
            } else {
                CurPerstDataEnd = data + highBound;
            }

            for (const char *fetch = CurPerstData; fetch < CurPerstDataEnd + 64; fetch += 64)
                Y_PREFETCH_READ(fetch, 3);
        }
        CurPerst = newPerst;
        Cur0 = CurPerstData;
    }

    void InternalRestart() {
        ChangeCurrentPerst<true>(HitsForRead.GetFirstPerst());
        Decoder.Reset();
        EofFlag = false;
        Decoder.ReadHeader(Cur0);
        if (HitsForRead.GetCount() < 8)
            Decoder.FallBackDecode(Cur0, HitsForRead.GetCount());

        Next();
        assert(EofFlag || Current() != START_POINT);
    }

    void SetEndOfHits() {
        EofFlag = true;
        Decoder.SetCurrent(MaxValue());
    }

    void Initialize() {
        Decoder.SetFormat(HitsForRead.GetHitFormat());
        Restart();
    }

private:
    TDecoder       Decoder;
    THitsForRead   HitsForRead;
    TBlob          PerstBlob;
    const char    *Cur0;
    bool           EofFlag;
    const YxPerst *CurPerst;
    const char*    CurPerstData;
    const char*    CurPerstDataEnd;
};

extern template class TPosIterator<>;

class IKeyPosScanner {
public:

    enum EContinueLogic { clStop, clCheckDocids, clSkip };

    virtual ~IKeyPosScanner() {

    }

    virtual const char* GetStartPos() const = 0;

    virtual EContinueLogic TestKey(const char* key) = 0;
    virtual void ProcessPos(const char* key, const TPosIterator<>& it) = 0;

};

template<typename TDecoder>
void TPosIterator<TDecoder>::Init(const IKeysAndPositions& index, i64 offset, ui32 length, i64 count, READ_HITS_TYPE type)
{
    HitsForRead.ReadHits(index, offset, length, count, type);
    HitsForRead.SetHitFormat(DetectHitFormat(index.GetVersion()));
    Initialize();
}

template<typename TDecoder>
TPosIterator<TDecoder>::TPosIterator(const IKeysAndPositions& index, const char* key, READ_HITS_TYPE type, TRequestContext *cache)
  : Cur0(nullptr)
  , EofFlag(true)
  , CurPerst(&YxPerst::NullSubIndex)
  , CurPerstData(nullptr)
  , CurPerstDataEnd(nullptr)
{
    Init(index, key, type, cache);
}

template<typename TDecoder>
bool TPosIterator<TDecoder>::Init(const IKeysAndPositions& index, const char* key, READ_HITS_TYPE type, TRequestContext *cache)
{
    TRequestContext rc;
    const YxRecord *rec = ExactSearch(&index, cache ? *cache : rc, key);
    if (rec) {
        Init(index, rec->Offset, rec->Length, rec->Counter, type);
        return true;
    } else {
        return false;
    }
}

template<typename TDecoder>
void TPosIterator<TDecoder>::Init(const ::TIndexInfo& ii, TBlob& parent, i64 parent_offset,
                                  i64 offset, ui32 length, i64 count)
{
    HitsForRead.AttachTo(ii, parent, parent_offset, offset, length, count);
    Initialize();
}

template<typename TDecoder>
void TPosIterator<TDecoder>::Init(const THitsForRead& other)
{
    HitsForRead = other;
    Initialize();
}

template<typename TDecoder>
void TPosIterator<TDecoder>::SkipAndCount(SUPERLONG to, i64& count)
{
    assert(!EofFlag);

    const YxPerst* NewPerst = CurPerst;
    while (NewPerst->Sum < to) {
        NewPerst = Advance(NewPerst, 1, TDecoder::GetPerstSize(HitsForRead));
        count += TDecoder::GetSubIndexStep(HitsForRead);
    }
    if (NewPerst != CurPerst) {
        ChangeCurrentPerst<false>(NewPerst);
        assert(Decoder.GetCurrent() < to);
        count = count - count % TDecoder::GetSubIndexStep(HitsForRead) - 1;
    }

    while (Decoder.GetCurrent() < to && Valid()) {
        Next();
        ++count;
    }
}

template<typename TDecoder>
void TPosIterator<TDecoder>::SkipToCount(const i64 countOld, const i64 countNew)
{
    assert(!EofFlag);

    if (countNew >= HitsForRead.GetCount() || countOld > countNew || countOld < 0) {
        SetEndOfHits();
        return;
    }

    i64 c = countOld;
    const YxPerst *NewPerst = CurPerst;
    if (NewPerst != &YxPerst::NullSubIndex) {
        i64 p1 = countOld / TDecoder::GetSubIndexStep(HitsForRead);
        i64 p2 = countNew / TDecoder::GetSubIndexStep(HitsForRead);
        if (p2 - p1 > 0) {
            NewPerst = Advance(CurPerst, p2 - p1, TDecoder::GetPerstSize(HitsForRead));  //We already checked p2 & p1 validity
            c += (p2 - p1) * TDecoder::GetSubIndexStep(HitsForRead);
        }
    }

    if (NewPerst != CurPerst) {
        ChangeCurrentPerst<false>(NewPerst);
        c = c - c % TDecoder::GetSubIndexStep(HitsForRead) - 1;
    }

    for (;c < countNew && Valid(); ++c) {
        Next();
    }
}

//////////////////////////////////////////////////////////////////////////
Y_FORCE_INLINE bool IsStrict(const THitsForRead &hitsForRead) {
    if (hitsForRead.GetSubIndexInfo().nPerstSize != 12)
        return false;
    if (hitsForRead.GetSubIndexInfo().nMinNeedSize != 128)
        return false;
    if (hitsForRead.GetSubIndexInfo().nSubIndexStep != 64)
        return false;
    return true;
}
