#pragma once


#include "map_block.h"
#include <kernel/xref/enums/bits.h>
#include <kernel/struct_codegen/reflection/floats.h>

static const size_t DMapBlockSize = 8;

// Информация о документе, сохраняемая в dmap-файле
class TDMapInfo {
public:
    // 32 бита
    ui32 WordCountSum:(XMapLinkBits + XMapWordBits); // Сумма количеств слов во всех ссылках на документ
    ui32 Reserved1 : (8*sizeof(ui32) - XMapLinkBits - XMapWordBits);
    // 16 бит
    ui32 UniqueTextCount:XMapLinkBits; // Количество уникальных текстов ссылок на документ
    ui32 Reserved2 : (8*sizeof(ui16) - XMapLinkBits);
    // 16 бит
private:
    ui32 RelativeWeightSum:16; // Сумма относительных весов ссылок на документ

public:
    float GetRelativeWeightSum() const {
        return GetFloatFromSf16(RelativeWeightSum);
    }
    void SetRelativeWeightSum(float f) {
        RelativeWeightSum = GetSf16FromFloat(f);
    }

    TDMapInfo() {
        Reserved1 = 0;
        Reserved2 = 0;
    }
};

typedef TMapBlock<TDMapInfo, DMapBlockSize> TDMapBlock;
