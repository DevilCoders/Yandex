#include "region.h"

#include <kernel/search_types/search_types.h>

#include <util/generic/array_size.h>
#include <util/generic/hash.h>
#include <util/generic/singleton.h>
#include <util/generic/yexception.h>

#include <array>

namespace {
    // country IDs from geobase
    static constexpr TCateg MAPPING[] = {
        END_CATEG,
        10090,
        10054,
        20826,
        10088,
        21182,
        20856,
        93,
        168,
        21536,
        211,
        113,
        167,
        21325,
        10532,
        10091,
        21019,
        149,
        114,
        21544,
        20869,
        21546,
        21550,
        10015,
        10057,
        21239,
        94,
        21559,
        20274,
        115,
        21165,
        21214,
        20975,
        20736,
        95,
        21326,
        21007,
        21331,
        20862,
        134,
        10029,
        21191,
        21297,
        21198,
        21574,
        21131,
        20733,
        10083,
        10017,
        21538,
        20574,
        125,
        20762,
        203,
        21475,
        20746,
        20917,
        21562,
        20785,
        1056,
        20769,
        21045,
        20989,
        179,
        20768,
        101519,
        10030,
        123,
        124,
        21451,
        21330,
        21137,
        21010,
        169,
        96,
        20802,
        10089,
        246,
        21567,
        21426,
        20747,
        20968,
        20818,
        21143,
        21477,
        21321,
        21175,
        116,
        10064,
        994,
        10095,
        10536,
        20572,
        10063,
        181,
        205,
        10013,
        137,
        10535,
        159,
        21223,
        21572,
        10537,
        207,
        20972,
        206,
        10538,
        21261,
        21278,
        10023,
        10067,
        117,
        21203,
        10068,
        20854,
        21151,
        10097,
        10098,
        21004,
        10069,
        101521,
        21349,
        21241,
        20271,
        208,
        10070,
        10099,
        21610,
        37176,
        10020,
        21235,
        10100,
        21217,
        21582,
        10101,
        118,
        21584,
        139,
        21231,
        21339,
        20741,
        98542,
        98539,
        10104,
        119,
        21586,
        10102,
        21589,
        98552,
        21299,
        20739,
        20992,
        21156,
        10108,
        120,
        10074,
        20764,
        21486,
        10077,
        225,
        21371,
        20789,
        21042,
        21395,
        20754,
        20860,
        20790,
        21199,
        10540,
        21441,
        180,
        10022,
        21219,
        10105,
        109724,
        121,
        122,
        20915,
        21227,
        10021,
        135,
        108137,
        204,
        10109,
        20957,
        21344,
        21251,
        127,
        126,
        10542,
        209,
        21208,
        995,
        21570,
        21580,
        21578,
        21553,
        21171,
        21599,
        21187,
        10024,
        983,
        170,
        21595,
        21601,
        21230,
        187,
        210,
        102,
        84,
        21289,
        171,
        21556,
        21359,
        21184,
        10093,
        21551,
        21196,
        20954
    };

    static constexpr size_t MAPPING_SIZE = Y_ARRAY_SIZE(MAPPING);

    static_assert(
        NCountry::ECountry_ARRAYSIZE == MAPPING_SIZE,
        "You must define region id for each member of NCountry::ECountry"
    );

    static_assert(
        0 == NCountry::ECountry_MIN,
        "Smallest value of NCountry::ECountry must be zero"
    );

    class THelper {
    public:
        THelper() {
            // expected that all values from 0 to ECountry_ARRAYSIZE - 1 are valid enums
            RegionIdToEnum_.reserve(MAPPING_SIZE);
            for (int country = 0; country < NCountry::ECountry_ARRAYSIZE; ++country) {
                EnumToRegionId_[country] = MAPPING[country];
                if (RegionIdToEnum_.contains(EnumToRegionId_[country])) {
                    ythrow yexception() << '"' << RegionIdToEnum_[country]
                                        << "\" was mentioned at least twice";
                }

                RegionIdToEnum_[EnumToRegionId_[country]] = static_cast<NCountry::ECountry>(country);
            }
        }

        inline TCateg RegionId(const NCountry::ECountry country) const {
            if (Y_UNLIKELY(!NCountry::ECountry_IsValid(country))) {
                ythrow yexception{};
            }

            return EnumToRegionId_.at(country);
        }

        inline NCountry::ECountry Enum(const TCateg regionId) const {
            if (const auto* const ptr = MapFindPtr(RegionIdToEnum_, regionId)) {
                return *ptr;
            } else {
                ythrow yexception();
            }
        }

        inline bool Enum(const TCateg regionId, NCountry::ECountry& country) const {
            if (const auto* const ptr = MapFindPtr(RegionIdToEnum_, regionId)) {
                country = *ptr;
                return true;
            }

            return false;
        }

    private:
        THashMap<TCateg, NCountry::ECountry> RegionIdToEnum_;
        std::array<TCateg, NCountry::ECountry_ARRAYSIZE> EnumToRegionId_;
    };
}  // namespace

TCateg NCountry::ToRegionId(const NCountry::ECountry country) {
    return Default<THelper>().RegionId(country);
}

NCountry::ECountry FromRegionId(const TCateg regionId) {
    return Default<THelper>().Enum(regionId);
}

bool NCountry::TryFromRegionId(const TCateg regionId, ECountry& country) {
    return Default<THelper>().Enum(regionId, country);
}
