#pragma once

#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/bitops.h>

#ifdef RT_HITS_CODER_INTERNAL_OPTIMIZATION
#include <util/string/printf.h>
#endif

#define RT_HITS_CODER_COUNT_BITS 4
#define RT_HITS_CODER_COUNT_MAX (0x1lu << RT_HITS_CODER_COUNT_BITS)
#define RT_HITS_CODER_COUNT_MASK ((ui64)(RT_HITS_CODER_COUNT_MAX - 1))
#define RT_HITS_CODER_GROUNDS_MAX (0x1lu << (11 - RT_HITS_CODER_COUNT_BITS))
#define RT_HITS_CODER_GROUNDS_MASK ((ui64)((0x1lu << 11) - RT_HITS_CODER_COUNT_MAX))

#ifdef RT_HITS_CODER_INTERNAL_OPTIMIZATION
struct TRTBitsMap {
    bool KeepBreaks;
    bool KeepWords;
    bool KeepRelev;
    bool KeepForms;

    ui16 DataShift1;
    ui16 DataShift2;

    ui64 DataMask1;
    ui64 DataMask2;

    TRTBitsMap()
        : KeepBreaks(1)
        , KeepWords(1)
        , KeepRelev(1)
        , KeepForms(1)
        , DataShift1(0)
        , DataShift2(0)
        , DataMask1((ui64)0x0000000000000000llu)
        , DataMask2((ui64)0x0000000000000000llu)
    { }

    TRTBitsMap(
        bool keepBreaks,
        bool keepWords,
        bool keepRelev,
        bool keepForms,
        ui64 dataMask1,
        ui64 dataMask2,
        ui16 dataShift1,
        ui16 dataShift2
    )
        : KeepBreaks(keepBreaks)
        , KeepWords(keepWords)
        , KeepRelev(keepRelev)
        , KeepForms(keepForms)
        , DataShift1(dataShift1)
        , DataShift2(dataShift2)
        , DataMask1(dataMask1)
        , DataMask2(dataMask2)
    { }
    TString DebugString() const {
        return Sprintf("{b%s, w%s, r%s, f%s, d1=0x%016lx, d2=0x%016lx, d1s=%2u, d2s=%2u}",
            (KeepBreaks ? "+" : "-"),
            (KeepWords ? "+" : "-"),
            (KeepRelev ? "+" : "-"),
            (KeepForms ? "+" : "-"),
            (long unsigned)DataMask1,
            (long unsigned)DataMask2,
            DataShift1,
            DataShift2
        );
    }
};
#endif

struct TRTGroundData {
    const i32 ShortMask;
    const ui16 DataShift1;
    const ui16 DataShift2;
    const ui64 NotDataMask;
    const ui64 DataMask1;
    const ui64 DataMask2;

    TRTGroundData()
        : ShortMask(0)
        , DataShift1(0)
        , DataShift2(0)
        , NotDataMask((ui64)-1)
        , DataMask1(0)
        , DataMask2(0)
    {
        // Just garbage in memory
    }

    TRTGroundData(
        bool keepBreaks,
        bool keepWords,
        bool keepRelev,
        bool keepForms,
        ui64 dataMask1,
        ui64 dataMask2,
        ui16 dataShift1,
        ui16 dataShift2
    )
        : ShortMask((i32)((ui32)DOC_LEVEL_Max << DOC_LEVEL_Shift) |
            ((ui32)keepForms * (NFORM_LEVEL_Max << NFORM_LEVEL_Shift)) |
            ((ui32)keepRelev * (RELEV_LEVEL_Max << RELEV_LEVEL_Shift)) |
            ((ui32)keepWords * (WORD_LEVEL_Max << WORD_LEVEL_Shift)) |
            ((ui32)keepBreaks * (BREAK_LEVEL_Max << BREAK_LEVEL_Shift)))
        , DataShift1(dataShift1)
        , DataShift2(dataShift2)
        , NotDataMask(~((dataMask1 << dataShift1) | (dataMask2 >> dataShift2)))
        , DataMask1(dataMask1)
        , DataMask2(dataMask2)
    {
        Y_VERIFY((DataMask1 << dataShift1 >> dataShift1 == dataMask1) &&
            (DataMask2 >> dataShift2 << dataShift2 == dataMask2),
            "Data shifts invariant is broken!\n"
            "DataMask1 = 0x%016lx\n"
            "DataMask2 = 0x%016lx\n"
            "dataShift1 = %u\n"
            "dataShift2 = %u\n"
            "dataMask1 = 0x%016lx\n"
            "dataMask1 = 0x%016lx\n"
            "DataMask1 << dataShift1 >> dataShift1 = 0x%016lx\n"
            "DataMask2 >> dataShift2 << dataShift2 = 0x%016lx\n",
            (long unsigned)DataMask1,
            (long unsigned)DataMask2,
            dataShift1,
            dataShift2,
            (long unsigned)dataMask1,
            (long unsigned)dataMask2,
            (long unsigned)(DataMask1 << dataShift1 >> dataShift1),
            (long unsigned)(DataMask2 >> dataShift2 << dataShift2));
    }

    TRTGroundData(
        ui64 dataMask1,
        ui64 dataMask2,
        ui16 dataShift1,
        ui16 dataShift2
    )
        : ShortMask(-1)
        , DataShift1(dataShift1)
        , DataShift2(dataShift2)
        , NotDataMask(~((dataMask1 << dataShift1) | (dataMask2 >> dataShift2)))
        , DataMask1(dataMask1)
        , DataMask2(dataMask2)
    {
        Y_VERIFY((DataMask1 << dataShift1 >> dataShift1 == dataMask1) &&
            (DataMask2 >> dataShift2 << dataShift2 == dataMask2),
            "Data shifts invariant is broken!\n"
            "DataMask1 = 0x%016lx\n"
            "DataMask2 = 0x%016lx\n"
            "dataShift1 = %u\n"
            "dataShift2 = %u\n"
            "dataMask1 = 0x%016lx\n"
            "dataMask1 = 0x%016lx\n"
            "DataMask1 << dataShift1 >> dataShift1 = 0x%016lx\n"
            "DataMask2 >> dataShift2 << dataShift2 = 0x%016lx\n",
            (long unsigned)DataMask1,
            (long unsigned)DataMask2,
            dataShift1,
            dataShift2,
            (long unsigned)dataMask1,
            (long unsigned)dataMask2,
            (long unsigned)(DataMask1 << dataShift1 >> dataShift1),
            (long unsigned)(DataMask2 >> dataShift2 << dataShift2));
    }

#ifdef RT_HITS_CODER_INTERNAL_OPTIMIZATION
    TRTGroundData(const TRTBitsMap& bitsMap, size_t maxCount)
        : ShortMask((i32)((ui32)DOC_LEVEL_Max << DOC_LEVEL_Shift) |
            ((ui32)bitsMap.KeepForms * (NFORM_LEVEL_Max << NFORM_LEVEL_Shift)) |
            ((ui32)bitsMap.KeepRelev * (RELEV_LEVEL_Max << RELEV_LEVEL_Shift)) |
            ((ui32)bitsMap.KeepWords * (WORD_LEVEL_Max << WORD_LEVEL_Shift)) |
            ((ui32)bitsMap.KeepBreaks * (BREAK_LEVEL_Max << BREAK_LEVEL_Shift)))
        , DataShift1(bitsMap.DataShift1)
        , DataShift2(bitsMap.DataShift2)
        , NotDataMask(~( (bitsMap.DataMask1 << bitsMap.DataShift1) |
            (bitsMap.DataMask2 >> bitsMap.DataShift2) ))
        , DataMask1(bitsMap.DataMask1)
        , DataMask2(bitsMap.DataMask2)
    {
        Y_VERIFY(maxCount < RT_HITS_CODER_COUNT_MAX, "Something wrong!\n");
        ui64 data1 = (ui64)DataMask1 << DataShift1;
        ui64 data2 = (ui64)DataMask2 >> DataShift2;
        Y_VERIFY(!(data1 & data2),
            "Data interleaving invariant is broken:\n"
            "keepBreaks = %s\n"
            "keepWords = %s\n"
            "keepRelev = %s\n"
            "keepForms = %s\n"
            "data1Mask = 0x%016lx\n"
            "data2Mask = 0x%016lx\n"
            "data1BitsShift = %u\n"
            "data2BitsShift = %u\n",
            ((Mask & ((ui64)0x1 << BREAK_LEVEL_Shift)) ? "true" : "false"),
            ((Mask & ((ui64)0x1 << WORD_LEVEL_Shift)) ? "true" : "false"),
            ((Mask & ((ui64)0x1 << RELEV_LEVEL_Shift)) ? "true" : "false"),
            ((Mask & ((ui64)0x1 << NFORM_LEVEL_Shift)) ? "true" : "false"),
            (long unsigned)DataMask1,
            (long unsigned)DataMask2,
            DataShift1,
            DataShift2
        );
    }

    TRTBitsMap GenBitsMap() const {
        return TRTBitsMap(
            /*keepBreaks=*/Mask & ((ui64)0x1 << BREAK_LEVEL_Shift),
            /*keepWords=*/Mask & ((ui64)0x1 << WORD_LEVEL_Shift),
            /*keepRelev=*/Mask & ((ui64)0x1 << RELEV_LEVEL_Shift),
            /*keepForms=*/Mask & ((ui64)0x1 << NFORM_LEVEL_Shift),
            DataMask1,
            DataMask2,
            DataShift1,
            DataShift2
        );
    }

    TString DebugString() const {
        if (Mask == 0)
            return TString("{------------------------------------blank-----------------------------------}");
        return Sprintf("{b%s, w%s, r%s, f%s, d1=0x%016lx, d2=0x%016lx, d1s=%2u, d2s=%2u}",
            ((Mask & ((ui64)0x1 << BREAK_LEVEL_Shift)) ? "+" : "-"),
            ((Mask & ((ui64)0x1 << WORD_LEVEL_Shift)) ? "+" : "-"),
            ((Mask & ((ui64)0x1 << RELEV_LEVEL_Shift)) ? "+" : "-"),
            ((Mask & ((ui64)0x1 << NFORM_LEVEL_Shift)) ? "+" : "-"),
            (long unsigned)DataMask1,
            (long unsigned)DataMask2,
            DataShift1,
            DataShift2
        );
    }
    static void UpdateCounters(const SUPERLONG* hits, size_t count, size_t maxCount, size_t secondMaxCount, ui64 low11);
#endif
};

static_assert(sizeof(TRTGroundData) == 32, "expect sizeof(TRTGroundData) == 32");

struct TRTGroundDataSet {
    union {
        const TRTGroundData* GroundData;
        ui64 AlignmentHack;
    };
    const ui64 MaxCount;

    TRTGroundDataSet(const TRTGroundData* groundData, size_t start, size_t finish)
        : GroundData(groundData + start)
        , MaxCount(finish - start)
    { }
};

static_assert(sizeof(TRTGroundDataSet) == 16, "expect sizeof(TRTGroundDataSet) == 16");

struct TRTCoderData {
    const TRTGroundDataSet* DecoderData;
};

extern const TRTCoderData globalRTCoderData;

#define RT_HITS_CODER_GROUNDS_NUM 128

static_assert(RT_HITS_CODER_GROUNDS_NUM <= RT_HITS_CODER_GROUNDS_MAX, "expect RT_HITS_CODER_GROUNDS_NUM <= RT_HITS_CODER_GROUNDS_MAX");
