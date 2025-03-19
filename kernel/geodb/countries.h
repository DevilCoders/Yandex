#pragma once
/// @file countries.h Contains frequently used geo constants whose ids are in geodb


#include <kernel/search_types/search_types.h>

namespace NGeoDB {
    static constexpr TCateg RUSSIA_ID = 225;
    static constexpr TCateg UKRAINE_ID = 187;
    static constexpr TCateg BELARUS_ID = 149;
    static constexpr TCateg KAZAKHSTAN_ID = 159;
    static constexpr TCateg TURKEY_ID = 983;
    static constexpr TCateg USA_ID = 84;
    static constexpr TCateg GREAT_BRITAIN_ID = 102;
    static constexpr TCateg NETHERLANDS_ID = 118;
    static constexpr TCateg FRANCE_ID = 124;
    static constexpr TCateg GERMANY_ID = 96;
    static constexpr TCateg MOSCOW_ID = 213;
    static constexpr TCateg CIS_ID = 166;         // SNG countries - the abbreviation in English is CIS
    static constexpr TCateg EARTH_ID = 10000;

    static constexpr TCateg KUBR_IDS[] = {KAZAKHSTAN_ID, UKRAINE_ID, BELARUS_ID, RUSSIA_ID};

    inline bool IsCountryFromKUBR(const TCateg id) noexcept {
        for (const auto country : KUBR_IDS) {
            if (country == id) {
                return true;
            }
        }

        return false;
    }
} // namespace NGeoDB
