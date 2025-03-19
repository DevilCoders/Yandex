#pragma once

#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/generic/bitmap.h>


namespace NAddressFilter {

enum ETokenSubType
{
    TT_ERROR           = -1,
    TT_EMPTY            = 0,
    TT_HOUSE            = 1,
    TT_DESCRIPTOR       = 2,
    TT_BUILDING         = 3,
    TT_CITY             = 4,
    TT_LINE             = 5,
    TT_VO_11            = 6,
    TT_VO_21            = 7,
    TT_VO_22            = 8,

    TT_STREET_DICT_11   = 9,
    TT_STREET_DICT_21   = 10,
    TT_STREET_DICT_22   = 11,
    TT_STREET_DICT_31   = 12,
    TT_STREET_DICT_32   = 13,
    TT_STREET_DICT_33   = 14,
    TT_STREET_DICT_41   = 15,
    TT_STREET_DICT_42   = 16,
    TT_STREET_DICT_43   = 17,
    TT_STREET_DICT_44   = 18,

    TT_METRO_11         = 19,
    TT_METRO_21         = 20,
    TT_METRO_22         = 21,
    TT_METRO_31         = 22,
    TT_METRO_32         = 23,
    TT_METRO_33         = 24,
    TT_METRO_DESCR      = 25,
    TT_NAME_DESCR       = 26,

    TT_CITY_DICT_11     = 27,

    TT_CAPITAL          = 28,
    TT_NUMBER           = 29,
    TT_NUMBER_END       = 30,

    TT_STREET_DICT_11S   = 31,
    TT_STREET_DICT_21S   = 32,
    TT_STREET_DICT_22S   = 33,
    TT_STREET_DICT_31S   = 34,
    TT_STREET_DICT_32S   = 35,
    TT_STREET_DICT_33S   = 36,
    TT_STREET_DICT_41S   = 37,
    TT_STREET_DICT_42S   = 38,
    TT_STREET_DICT_43S   = 39,
    TT_STREET_DICT_44S   = 40,

    TT_MONTH             = 41,
    TT_ADDRESS           = 42,
    TT_DESCRIPTOR_NN     = 43,

    TT_CITY_DICT_21      = 44,
    TT_CITY_DICT_22      = 45
};

extern const TString TYPE_DESCRIPTIONS[];
extern const wchar16 ALLOWED_PUNKTUATION[];
extern size_t ALLOWED_PUNKTUATION_SIZE;


typedef TBitMap<46> TTokenType;
typedef std::pair<size_t, size_t> TAddressPosition;

TString GetDebugTokenType(const TTokenType& tokenType);

}
