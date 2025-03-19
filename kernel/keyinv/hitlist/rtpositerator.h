#pragma once

#include "hitread.h"
#include "hits_coders.h"

#include <kernel/keyinv/hitlist/memory/rt_hits_block.h>
#include <kernel/keyinv/hitlist/memory/rt_hits_coders.h>

namespace NMemorySearch {

// Only one of decoders can contain all hits of document!
class TRTPosIterator {
private:
    TRTDecoder Decoders[MAX_RT_HITS_BLOCKS];
    size_t CurrentDecoderId;
    size_t Length;

    void SelectDecoder() noexcept {
        SUPERLONG minHit = Decoders[0].GetCurrent();
        CurrentDecoderId = 0;
        for (size_t i = 1; i < MAX_RT_HITS_BLOCKS; ++i) {
            SUPERLONG hit = Decoders[i].GetCurrent();
            if (hit < minHit) {
                CurrentDecoderId = i;
                minHit = hit;
            }
        }
    }

public:
    TRTPosIterator() noexcept
        : CurrentDecoderId(0)
        , Length(0)
    { }

    typedef SUPERLONG value_type;

    static Y_FORCE_INLINE bool CanFold() noexcept {
        return false;
    }
    static Y_FORCE_INLINE SUPERLONG MaxValue() noexcept {
        return TRTDecoder::MaxValue;
    }
    Y_FORCE_INLINE SUPERLONG Current() const noexcept {
        return Decoders[CurrentDecoderId].GetCurrent();
    }
    SUPERLONG operator*() noexcept {
        return Current();
    }
    ui32 Doc() const noexcept {
        return TWordPosition::Doc(Current());
    }
    ui32 DocLength() const noexcept {
        return TWordPosition::DocLength(Current());
    }
    void SkipAndInDocCount(SUPERLONG to, i64& count) noexcept {
        SUPERLONG docBits = (SUPERLONG)((ui64)TWordPosition::Doc(to) << DOC_LEVEL_Shift);
        SkipTo(docBits);
        if (TWordPosition::Doc(Current()) == TWordPosition::Doc(to)) {
            Decoders[CurrentDecoderId].SkipAndCount(to, count);
            SelectDecoder();
        }
    }
    void SkipToInDocCount(SUPERLONG to, const i64& count) noexcept {
        SUPERLONG docBits = (SUPERLONG)((ui64)TWordPosition::Doc(to) << DOC_LEVEL_Shift);
        SkipTo(docBits);
        if (TWordPosition::Doc(Current()) == TWordPosition::Doc(to)) {
            Decoders[CurrentDecoderId].SkipCount(count);
            SelectDecoder();
        }
    }
    void Init(const THitsForRead& other) noexcept {
        Y_ASSERT(HIT_FMT_RT == other.GetHitFormat());
        const TRTImmediateBlock& block = *reinterpret_cast<const TRTImmediateBlock*>(other.GetData().AsCharPtr());
        TRTImmediateBlock::TRefData refData = block.GetRefData();

        size_t numDecoders = 0;
        if (refData.IsPointer()) {
            ui32 length1 = refData.GetLength();
            Length = length1;
            while (true) {
                // Calculate length for this level
                TRTImmediateBlock::TRefData newRefData = refData;
                TRTRefVariableBlock::Next(newRefData);
                ui32 length2 = 0;
                bool nextPointer = newRefData.IsPointer();
                if (nextPointer) {
                    length2 = newRefData.GetLength();
                }
                ui32 length = length1 - length2;

                // Add decoder
                const ui64* start = newRefData.GetData();
                const ui64* end = start + length / sizeof(ui64);
                TRTDecoder& decoder = Decoders[numDecoders++];
                decoder.Set(start, end);
                decoder.Next();

                if (!nextPointer) {
                    break;
                }
                // Moving precalculated data to next step
                length1 = length2;
                refData = newRefData;
            }
        } else if (Y_LIKELY(!refData.IsInitState())) {
            const ui64* start = refData.GetData();
            const ui64* end = refData.GetDataEnd();
            Length = end - start;
            TRTDecoder& decoder = Decoders[numDecoders++];
            decoder.Set(start, end);
            decoder.Next();
        }
        SelectDecoder();
    }
    void Init(const IKeysAndPositions& index, i64 offset, ui32 length, i64 count, READ_HITS_TYPE type) {
        THitsForRead hitsForRead;
        hitsForRead.ReadHits(index, offset, length, count, type);
        hitsForRead.SetHitFormat(DetectHitFormat(index.GetVersion()));
        Init(hitsForRead);
    }
    bool Init(const IKeysAndPositions& index, const char* key, READ_HITS_TYPE type) {
        TRequestContext rc;
        const YxRecord *rec = ExactSearch(&index, rc, key);
        if (rec) {
            Init(index, rec->Offset, rec->Length, rec->Counter, type);
            return true;
        } else {
            return false;
        }
    }
    bool Valid() const noexcept {
        return Current() != MaxValue();
    }
    void Y_FORCE_INLINE Touch() const noexcept {
        // Do nothing
    }
    void SkipTo(SUPERLONG to) noexcept {
        while (true) {
            size_t currentDecoderId = CurrentDecoderId;
            Decoders[currentDecoderId].SkipToOrBreakDown(to);
            SelectDecoder();
            if (currentDecoderId != CurrentDecoderId && Decoders[CurrentDecoderId].GetCurrent() < to) {
                continue;
            }
            break;
        }
    }
    template <class TSmartSkip>
    void SkipTo(SUPERLONG to, const TSmartSkip &) noexcept {
        SkipTo(to);
    }
    void SkipCount(size_t count) noexcept {
        // TODO: optimize
        for (; count; --count) {
            Next();
        }
    }
    ui64 GetCurrentPerst() const noexcept {
        size_t length = 0;
        for (size_t i = 0; i < MAX_RT_HITS_BLOCKS; ++i) {
            length += Decoders[i].GetLength();
        }
        return (Length - length) / (8 * 3); // Expecting that hit is packed to 3 bytes
    }
    ui64 GetLastPerst() const noexcept {
        return Length / (8 * 3); // Expecting that hit is packed to 3 bytes
    }
    [[noreturn]] void Y_FORCE_INLINE NextChunk(TFullPosition*& /*positions*/, SUPERLONG /*bound*/, const ui8* /*formPriorities*/, const ui64* /*formMasks*/, ui64& /*mask*/) {
        Y_FAIL("Not implemented!\n");
    }

    void SaveIndexAccess(IOutputStream* /*rh*/) const {
        //this is used for iterator's cache (aka TReqBundleIteratorsHashers) but our GetReqBundleHashers is always zero
        Y_ASSERT(false);
    };
    void RestoreIndexAccess(IInputStream* /*rh*/, const IKeysAndPositions& /*index*/, READ_HITS_TYPE type = RH_DEFAULT) {
        Y_UNUSED(type);
        //this is used for iterator's cache (aka TReqBundleIteratorsHashers) but our GetReqBundleHashers is always zero
        Y_ASSERT(false);
    };
    TRTPosIterator& operator++() {
        Next();
        return *this;
    }
    void Y_FORCE_INLINE Next() noexcept {
        Decoders[CurrentDecoderId].Next();
        SelectDecoder();
    }
};

} // namespace NMemorySearch

using TRTPosIterator = NMemorySearch::TRTPosIterator;
