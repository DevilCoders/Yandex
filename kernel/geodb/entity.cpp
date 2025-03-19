#include "entity.h"

#include <util/generic/yexception.h>
#include <util/generic/array_size.h>

namespace NGeoDB {
    static const char* const ENTITY_TO_STR[] = {
        "WORLD",
        "CONTINENT",
        "REGION",
        "COUNTRY",
        "FEDERAL_DISTRICT",
        "CONSTITUENT_ENTITY",
        "CITY",
        "TOWN",
        "CITY_AREA",
        "SUBWAY_STATION",
        "CONSTITUENT_ENTITY_AREA",
        "AIRPORT",
        "OVERSEAS_TERRITORY",
        "CITY_SUBAREA",
        "LIGHT_RAILWAY_STATION",
        "RURAL_SETTLEMENT"
    };
    // no need ARRAY_SIZE() + 1 since there is no description for UNKNOWN type
    static_assert(Y_ARRAY_SIZE(ENTITY_TO_STR) == EType_ARRAYSIZE, "ENTITY_TO_STR has to have all the strings for each EType");

    static const TEntityTypeWeight ENTITY_TYPE_WEIGHT[] = {
        0,  //    WORLD                   =  0;
        1,  //    CONTINENT               =  1;
        2,  //    REGION                  =  2;
        3,  //    COUNTRY                 =  3;
        5,  //    FEDERAL_DISTRICT        =  4;
        6,  //    CONSTITUENT_ENTITY      =  5;
        8,  //    CITY                    =  6;
        9,  //    TOWN                    =  7;
        10, //    CITY_AREA               =  8;
        13, //    SUBWAY_STATION          =  9;
        7,  //    CONSTITUENT_ENTITY_AREA = 10;
        12, //    AIRPORT                 = 11;
        4,  //    OVERSEAS_TERRITORY      = 12;
        11, //    CITY_SUBAREA            = 13;
        14, //    LIGHT_RAILWAY_STATION   = 14;
        9   //    RURAL_SETTLEMENT        = 15;
    };
    // no need ARRAY_SIZE() + 1 since there is no weight for UNKNOWN type
    static_assert(Y_ARRAY_SIZE(ENTITY_TYPE_WEIGHT) == EType_ARRAYSIZE, "Not all weights defined for EType");

    TEntityTypeWeight EntityTypeToWeight(const EType type) {
        Y_ENSURE(EType::UNKNOWN != type && EType_IsValid(type),
               "No weight for this type: " << static_cast<int>(type));
        return ENTITY_TYPE_WEIGHT[type];
    }

    bool TryEntityTypeToWeight(const EType type, TEntityTypeWeight& weight) {
        if (Y_UNLIKELY(!(EType::UNKNOWN != type && EType_IsValid(type))))
            return false;

        weight = ENTITY_TYPE_WEIGHT[type];
        return true;
    }

} // namespace NGeoDB

// This function is for debug purposes only
// it is useful to Cerr << type;
template <>
void Out<NGeoDB::EType>(IOutputStream& out, TTypeTraits<NGeoDB::EType>::TFuncParam type) {
    const int intType = static_cast<int>(type);
    if (Y_LIKELY(NGeoDB::EType_IsValid(type))) {
        out << NGeoDB::ENTITY_TO_STR[type] << ":{id:" << intType << ",weight:";
        NGeoDB::TEntityTypeWeight weight;
        if (TryEntityTypeToWeight(type, weight))
            out << static_cast<int>(weight);
        else
            out << "unknown";
        out << '}';
    }
    else
        out << "Invalid:{id:" << intType << ",weight:unknown}";
}
