#pragma once

#include "query.h"
#include "feature.h"

#include <util/string/builder.h>

namespace NTextMachine {
    template <ERegionClassType RegionClass>
    struct TQuerySet {
        static_assert(RegionClass != TRegionClass::RegionClassMax, "incorrect region class");

        static ERegionClassType GetRegionClass() {
            return RegionClass;
        }

        static bool Has(const TQueryType& type) {
            return NLingBoost::IsRegionClassSubSet(type.RegionClass, RegionClass);
        }
        static void UpdateFeatureId(TFFId& id) {
            if (RegionClass != TRegionClass::Any) {
                id.Set(RegionClass);
            }
        }

        static TString ToString() {
            return TStringBuilder{}
                << "RegionClass: " << RegionClass;
        }
    };

    using TAnyQuery = TQuerySet<TRegionClass::Any>;
    using TWorldQuery = TQuerySet<TRegionClass::World>;
    using TCountryQuery = TQuerySet<TRegionClass::Country>;
} // NTextMachine
