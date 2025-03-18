#include "telephonoid.h"

#include <util/charset/wide.h>
#include <util/string/cast.h>

////
//  TTelephonoidChunk
////

ui16 TTelephonoidChunk::GetSpaceType(ui16 types, wchar16 ch) {
    ui16 flag = 0;
    switch (ch) {
        case CSpace:
        case CNonBreakingSpace:
            return types | DSpace;
        case CPlus:
            flag = DPlus;
            break;
        case CDash:
            flag = DDash;
            break;
        case COpeningParen:
            flag = DOpeningParen;
            break;
        case CClosingParen:
            flag = DClosingParen;
            break;
        default:
            return types | DBadChar;
    }
    if (types & flag) {
        return types | DBad;
    }
    return types | flag;
}

ui16 TTelephonoidChunk::GetRightPhoneSpacesTypes(TWtringBuf spaces) {
    ui16 types = 0;
    for (size_t i = spaces.size(); i-- > 0;) {
        types |= GetSpaceType(types, *(spaces.data() + i));
        if (types & DBadChar)
            return types ^ DBadChar;
    }
    return types;
}

ui16 TTelephonoidChunk::GetLeftPhoneSpacesTypesUntilError(TWtringBuf spaces) {
    ui16 types = 0;
    for (ui32 i = 0; i < spaces.size(); i++) {
        types |= GetSpaceType(types, *(spaces.data() + i));
        if (types & DBadChar) {
            return types;
        }
    }
    return types;
}

void TTelephonoidChunk::SpaceToString(TString& res, ui16 types) {
    if (0 == types || TTelephonoidChunk::DSpace == types) {
        res.append('_');
    } else if (types & TTelephonoidChunk::DBad) {
        res.append('?');
    } else {
        if (types & TTelephonoidChunk::DClosingParen) {
            res.append(')');
        }
        if (types & TTelephonoidChunk::DDash) {
            res.append('-');
        }
        if (types & TTelephonoidChunk::DOpeningParen) {
            res.append('(');
        }
        if (types & TTelephonoidChunk::DPlus) {
            res.append('+');
        }
    }
}

////
//  TTelephonoid
////

bool TTelephonoid::HasGoodDelimiters() const {
    if (Chunks[0].PrecedingSpacesTypes & (TTelephonoidChunk::DDash | TTelephonoidChunk::DClosingParen))
        return false;

    for (ui32 i = 0; i < Chunks.size(); i++) {
        if (Chunks[i].PrecedingSpacesTypes & TTelephonoidChunk::DBad)
            return false;
    }

    return true;
}

size_t TTelephonoid::NumCount() const {
    size_t maxLength = 0;
    for (size_t i = 0; i < Chunks.size(); i++) {
        maxLength += Chunks[i].Token.length();
    }
    return maxLength;
}

TString TTelephonoid::FullNumberToString() const {
    TString s;
    s.reserve(NumCount() + 1);
    for (size_t i = 0; i < Chunks.size(); i++) {
        s.append(WideToUTF8(Chunks[i].Token));
    }
    return s;
}

TString TTelephonoid::StructureToString() const {
    TString s;
    if (!HasGoodDelimiters())
        return s;
    for (size_t i = 0; i < Chunks.size(); i++) {
        TTelephonoidChunk::SpaceToString(s, Chunks[i].PrecedingSpacesTypes);
        s.append(Chunks[i].Token.length(), '#');
    }
    return s;
}
