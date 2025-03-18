#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

////
//  TTelephonoidChunk
////

class TTelephonoidChunk {
public:
    TWtringBuf Token;
    size_t TokenPos;
    ui16 PrecedingSpacesTypes;
    enum ECharCode {
        CSpace = 0x20,
        COpeningParen = 0x28,
        CClosingParen = 0x29,
        CPlus = 0x2B,
        CDash = 0x2D,
        CNonBreakingSpace = 0xA0
    };

    enum EDelimiterType {
        DPlus = 1 << 1,
        DDash = 1 << 2,
        DOpeningParen = 1 << 3,
        DClosingParen = 1 << 4,
        DBadChar = 1 << 6,
        DBad = 1 << 7,
        DSpace = 1 << 8,
    };

public:
    static ui16 GetSpaceType(ui16 types, wchar16 ch);
    static ui16 GetRightPhoneSpacesTypes(TWtringBuf spaces);
    static ui16 GetLeftPhoneSpacesTypesUntilError(TWtringBuf spaces);
    static void SpaceToString(TString& res, ui16 types);
};

////
//  TTelephonoid
////

class TTelephonoid {
public:
    TVector<TTelephonoidChunk> Chunks;

public:
    inline void Clear() {
        Chunks.clear();
    }
    bool HasGoodDelimiters() const;
    size_t NumCount() const;
    TString FullNumberToString() const;
    TString StructureToString() const;
};
