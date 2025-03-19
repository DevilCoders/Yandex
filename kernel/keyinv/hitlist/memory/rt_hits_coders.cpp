#include "rt_hits_coders.h"
#include "rt_hits_coders_internal.h"

TRTCoder::TRTCoder() noexcept
    : DecoderData(globalRTCoderData.DecoderData)
{ }

TRTDecoder::TRTDecoder() noexcept
    : Start(nullptr)
    , End(nullptr)
    , CurrentHit(MaxValue)
    , DecoderData(globalRTCoderData.DecoderData)
{ }

ui64* TRTCoder::Output(const SUPERLONG*& hits, ui64*__restrict dst, size_t& count) const noexcept {
#ifdef RT_HITS_CODER_INTERNAL_OPTIMIZATION
    ui64 secondMaxCount = 0;
#endif
    ui64 maxCount = 0;
    ui64 bestI = 0;

    const size_t groundStep = sizeof(TRTGroundDataSet);
    for (ui64 i = 0; i < RT_HITS_CODER_GROUNDS_MAX * groundStep; i += groundStep) {
        ui64 prevHit = (ui64)hits[0];
        const TRTGroundDataSet& groundDataSet = *(const TRTGroundDataSet*)((const char*)DecoderData + i);
        size_t localMaxCount = groundDataSet.MaxCount;
#ifdef RT_HITS_CODER_INTERNAL_OPTIMIZATION
        if (!localMaxCount)
            break;
        if (localMaxCount < secondMaxCount)
            break;
#else
        if (localMaxCount < maxCount)
            break;
#endif
        if (localMaxCount >= count)
            localMaxCount = count - 1;
        ui64 j;
        for (j = 0; j < localMaxCount; j++) {
            const TRTGroundData& groundData = groundDataSet.GroundData[j];

            ui64 currentHit = (ui64)hits[j + 1];
            i64 mask = (i64)groundData.ShortMask;
            ui64 notDataMask = groundData.NotDataMask;
            ui64 delta = currentHit - (prevHit & mask);

            if (delta & notDataMask)
                break;

            prevHit = currentHit;
        }
        ui64 gotCount = j;
        if (gotCount >= maxCount) {
#ifdef RT_HITS_CODER_INTERNAL_OPTIMIZATION
            secondMaxCount = maxCount;
#endif
            bestI = i;
            maxCount = gotCount + 1;
#ifndef RT_HITS_CODER_INTERNAL_OPTIMIZATION
            if (j == localMaxCount || maxCount == count)
                break;
#else
        } else {
            if (gotCount >= secondMaxCount)
                secondMaxCount = gotCount + 1;
#endif
        }
    }
    ui64 data = 0;

    ui64 prevHit = (ui64)hits[0];
    static_assert(sizeof(TRTGroundDataSet) == 16, "expect sizeof(TRTGroundDataSet) == 16"); // Just to ensure that " | bestI | " is correct code.
    ui64 header = (prevHit << 11) | bestI | (maxCount - 1);

    const TRTGroundDataSet& groundDataSet = *(const TRTGroundDataSet*)((const char*)DecoderData + bestI);
    const TRTGroundData* groundDataPtr = groundDataSet.GroundData;

    for (ui64 j = 0; j + 1 < maxCount; j++) {
        const TRTGroundData& groundData = groundDataPtr[j];
        ui64 currentHit = (ui64)hits[j + 1];
        i64 mask = (i64)groundData.ShortMask;
        ui16 dataShift1 = groundData.DataShift1;
        ui16 dataShift2 = groundData.DataShift2;
        ui64 dataMask1 = groundData.DataMask1;
        ui64 dataMask2 = groundData.DataMask2;
        ui64 delta = currentHit - (prevHit & mask);
        ui64 data1 = (delta >> dataShift1) & dataMask1;
        ui64 data2 = (delta << dataShift2) & dataMask2;
        data |= data1;
        data |= data2;
        prevHit = currentHit;
    }
#ifdef RT_HITS_CODER_INTERNAL_OPTIMIZATION
    TRTGroundData::UpdateCounters(hits, count, maxCount, secondMaxCount, header & (((ui64)0x1 << 11) - 1));
    Y_VERIFY(maxCount, "At least one hit must be encoded!\n"
        "count = %d\nmaxCount = %d\nhits[0] = 0x%016lx\n",
        (int)count, (int)maxCount, (long unsigned)hits[0]);
    Y_VERIFY(count <= 1 || maxCount > 1, "At least two hits must be encoded if exist!\n"
        "count = %d\nmaxCount = %d\nhits[0] = 0x%016lx\nhits[1] = 0x%016lx\n",
        (int)count, (int)maxCount, (long unsigned)hits[0], hits[1]);
#endif

    dst[0] = header;
    dst[1] = data;

    hits += maxCount;
    count -= maxCount;
    return dst + 2;
}
ui64* TRTCoder::OutputAll(const SUPERLONG* hits, ui64*__restrict dst, size_t count) const noexcept {
    while (count) {
        dst = Output(hits, dst, count);
    }
    return dst;
}

#define UNPACK_RT_BLOCK(addrVar, DataVar, GroundDataPtrVar) \
    currentHit = addrVar[0];\
    DataVar = addrVar[1];\
    count = currentHit & RT_HITS_CODER_COUNT_MASK;\
    size_t shift = currentHit & RT_HITS_CODER_GROUNDS_MASK;\
    const TRTGroundDataSet& groundDataSet = *(const TRTGroundDataSet*)((const char*)DecoderData + shift);\
    GroundDataPtrVar = groundDataSet.GroundData;\
    currentHit >>= 11;

#define NEXT_IN_RT_BLOCK(GroundDataPtrVar) \
    {\
        const TRTGroundData& groundData = *GroundDataPtrVar;\
        i64 mask = (i64)groundData.ShortMask;\
        ui16 dataShift1 = groundData.DataShift1;\
        ui16 dataShift2 = groundData.DataShift2;\
        ui64 dataMask1 = groundData.DataMask1;\
        ui64 dataMask2 = groundData.DataMask2;\
        ui64 maskedData1 = data & dataMask1;\
        ui64 maskedData2 = data & dataMask2;\
        currentHit &= mask;\
        currentHit += maskedData1 << dataShift1;\
        currentHit += maskedData2 >> dataShift2;\
        GroundDataPtrVar = &groundData + 1;\
        --count;\
    }

#define CHECK_RT_SKIP_END() \
    if (to <= currentHit) {\
        break;\
    }

void TRTDecoder::Next() noexcept {
    const ui64*__restrict start = Start;
    ui64 count = Count;

    if (Y_UNLIKELY(start == End)) {
        CurrentHit = MaxValue;
        return;
    }

    SUPERLONG currentHit;
    if (Y_UNLIKELY(!count)) {
        UNPACK_RT_BLOCK(start, Data, GroundDataPtr);
    } else {
        ui64 data = Data;
        currentHit = CurrentHit;
        NEXT_IN_RT_BLOCK(GroundDataPtr);
    }

    Count = count;
    CurrentHit = currentHit;

    if (Y_UNLIKELY(!count)) {
        Start = start + 2;
    }
}
void TRTDecoder::SkipAndCount(SUPERLONG to, i64& count) noexcept {
    while (CurrentHit < to && Start < End) {
        Next();
        ++count;
    }
}
void TRTDecoder::SkipToOrBreakDown(SUPERLONG to) noexcept {
    SUPERLONG currentHit = CurrentHit;
    ui64 toCmp = (ui64)to << 11;
    if (Y_UNLIKELY(to <= currentHit)) {
        // Win
        return;
    }
    const ui64* start = Start;
    const ui64* end = End;
    ui64 data = 0;
    size_t count;
    const TRTGroundData* groundDataPtr = nullptr;
    if (start + 2 < end && toCmp > *(ui64*)(start + 2)) {
        start += 2;
        count = 0;
        size_t shift = 32;
        while (true) {
            const ui64* newStart = start + shift;
            if (newStart >= end) {
                break;
            }
            if (toCmp <= *newStart) {
                break;
            }
            start = newStart;
            shift *= 16;
        }
        do {
            shift /= 2;
            const ui64* newStart = start + shift;
            if (newStart >= end) {
                continue;
            }
            if (toCmp <= *newStart) {
                continue;
            }
            start = newStart;
        } while (shift > 2);
    } else {
        data = Data;
        count = Count;
        groundDataPtr = GroundDataPtr;
        if (Y_UNLIKELY(start >= end)) {
            // Fail
            Start = start;
            CurrentHit = MaxValue;
            return;
        }
    }
    do {
        if (!count) {
            UNPACK_RT_BLOCK(start, data, groundDataPtr);
            if (!count)
                start += 2;
            CHECK_RT_SKIP_END();
            if (Y_UNLIKELY(start >= end)) {
                // Fail
                Start = start;
                CurrentHit = MaxValue;
                return;
            }
        }
        switch (count) {
            case 15:
                NEXT_IN_RT_BLOCK(groundDataPtr);
                CHECK_RT_SKIP_END();
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            case 14:
                NEXT_IN_RT_BLOCK(groundDataPtr);
                CHECK_RT_SKIP_END();
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            case 13:
                NEXT_IN_RT_BLOCK(groundDataPtr);
                CHECK_RT_SKIP_END();
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            case 12:
                NEXT_IN_RT_BLOCK(groundDataPtr);
                CHECK_RT_SKIP_END();
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            case 11:
                NEXT_IN_RT_BLOCK(groundDataPtr);
                CHECK_RT_SKIP_END();
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            case 10:
                NEXT_IN_RT_BLOCK(groundDataPtr);
                CHECK_RT_SKIP_END();
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            case  9:
                NEXT_IN_RT_BLOCK(groundDataPtr);
                CHECK_RT_SKIP_END();
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            case  8:
                NEXT_IN_RT_BLOCK(groundDataPtr);
                CHECK_RT_SKIP_END();
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            case  7:
                NEXT_IN_RT_BLOCK(groundDataPtr);
                CHECK_RT_SKIP_END();
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            case  6:
                NEXT_IN_RT_BLOCK(groundDataPtr);
                CHECK_RT_SKIP_END();
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            case  5:
                NEXT_IN_RT_BLOCK(groundDataPtr);
                CHECK_RT_SKIP_END();
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            case  4:
                NEXT_IN_RT_BLOCK(groundDataPtr);
                CHECK_RT_SKIP_END();
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            case  3:
                NEXT_IN_RT_BLOCK(groundDataPtr);
                CHECK_RT_SKIP_END();
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            case  2:
                NEXT_IN_RT_BLOCK(groundDataPtr);
                CHECK_RT_SKIP_END();
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            case  1:
                NEXT_IN_RT_BLOCK(groundDataPtr);
                start += 2;
                CHECK_RT_SKIP_END();
                if (Y_UNLIKELY(start >= end)) {
                    // Fail
                    Start = start;
                    CurrentHit = MaxValue;
                    return;
                }
                [[fallthrough]];
            case  0:
                UNPACK_RT_BLOCK(start, data, groundDataPtr);
                if (!count) {
                    start += 2;
                }
                break;
        };
    } while (false);
    // Win
    Start = start;
    CurrentHit = currentHit;
    Data = data;
    Count = count;
    GroundDataPtr = groundDataPtr;
}

void TRTDecoder::SkipCount(size_t countToSkip) noexcept {
    SUPERLONG currentHit = CurrentHit;
    const ui64* start = Start;
    const ui64* end = End;
    ui64 data = Data;
    size_t count = Count;
    const TRTGroundData* groundDataPtr = GroundDataPtr;
    while (countToSkip > count) {
        start += 2;
        if (start >= end) {
            // Fail
            Start = start;
            CurrentHit = MaxValue;
            return;
        }
        countToSkip -= count;

        UNPACK_RT_BLOCK(start, data, groundDataPtr);
        --countToSkip;
    }
    while (countToSkip) {
        NEXT_IN_RT_BLOCK(groundDataPtr);
        --countToSkip;
    }
    // Win
    CurrentHit = currentHit;
    Data = data;
    Count = count;
    GroundDataPtr = groundDataPtr;
    if (Y_UNLIKELY(!count))
        start += 2;
    Start = start;
}
