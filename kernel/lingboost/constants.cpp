#include "constants.h"
#include "enum_map.h"

#include <kernel/country_data/countries.h>

template<>
void Out<::NLingBoost::TRegionId>(IOutputStream& os, const ::NLingBoost::TRegionId& regionId)
{
    ::NLingBoost::ERegionClassType regionality = NLingBoost::GetRegionClassByRegionId(regionId);
    if (::NLingBoost::TRegionClass::Country == regionality) {
        const auto* data = GetCountryData(regionId);
        if (Y_LIKELY(data != nullptr)) {
            os << regionality << "(" << data->NationalDomain << ")";
            return;
        } else {
            Y_ASSERT(false);
        }
    } else if (::NLingBoost::TRegionClass::World == regionality) {
        os << regionality;
        return;
    }
    os << regionality << "(" << regionId.Value << ")";
}

template<>
::NLingBoost::TRegionId FromStringImpl(const char* data, size_t len) {
    TStringBuf text(data, len);

    i64 value = 0;
    if (TryFromString(text, value)) {
        return ::NLingBoost::TRegionId(value);
    }

    TStringBuf regionalityStr;
    ::NLingBoost::ERegionClassType regionality = ::NLingBoost::TRegionClass::RegionClassMax;
    Y_ENSURE_EX(text.NextTok('(', regionalityStr), TFromStringException()
        << "failed to parse regionality token from \"" << text << "\"");
    Y_ENSURE_EX(FromString(regionalityStr, regionality), TFromStringException()
        << "failed to parse regionality token from \"" << text << "\"");

    if (::NLingBoost::TRegionClass::World == regionality) {
        Y_ENSURE_EX(text.empty(), TFromStringException()
            << "unexpected trailing chars in \"" << text << "\"");

        return::NLingBoost::TRegionId::World();
    }

    Y_ENSURE_EX(text.find(')') != TStringBuf::npos, TFromStringException()
        << "closing ')' is expected but not found in \"" << text << "\"");

    TStringBuf valueStr;
    Y_ENSURE_EX(text.NextTok(')', valueStr), TFromStringException()
        << "failed to parse region id value from \"" << text << "\"");

    Y_ENSURE_EX(text.empty(), TFromStringException()
        << "unexpected trailing chars in \"" << text << "\"");

    if (!TryFromString(valueStr, value)) {
        TCateg country = NationalDomain2Country(TString(valueStr));
        Y_ENSURE_EX(END_CATEG != country, TFromStringException()
            << "failed to parse region id from \"" << text << "\"");
        value = country;
    }

    Y_ENSURE_EX(::NLingBoost::GetRegionClassByRegionId(::NLingBoost::TRegionId(value)) == regionality,
        TFromStringException()
            << "inconsistent region id and regionality: " << value << " and " << regionality);

    return ::NLingBoost::TRegionId(value);
}

template<>
bool TryFromStringImpl(const char* data, size_t len, ::NLingBoost::TRegionId& result) {
    try {
        result = FromStringImpl<::NLingBoost::TRegionId>(data, len);
        return true;
    } catch (TFromStringException&) {
        return false;
    }
}

namespace NLingBoost {
    namespace NPrivate {
        void FillExpansionTraitsByType(TCompactEnumMap<TExpansion, TExpansionTraits>& table) {
            table.Insert({
                  {EExpansionType::OriginalRequest, TExpansionTraits{false}},
                  {EExpansionType::RequestWithRegionName, TExpansionTraits{false}},
                  {EExpansionType::RequestWithCountryName, TExpansionTraits{false}},
                  {EExpansionType::RequestWithoutVerbs, TExpansionTraits{false}},
            });
        }

        const TExpansionTraits TExpansionTraitsByType::GetExpansionTraits(EExpansionType type) {
            if (this->IsKeyInRange(type)) {
                return (*this)[type];
            } else {
                return TExpansionTraits();
            }
        }
    } // NPrivate
    ERegionClassType GetRegionClassByRegionId(TRegionId region) {
        if (TRegionId::World() == region) {
            return TRegionClass::World;
        } else if (IsCountry(region)) {
            return TRegionClass::Country;
        } else if (region >= 0) {
            return TRegionClass::SmallRegion;
        } else {
            return TRegionClass::NotRegion;
        }
    }

    bool IsRegionClassSubSet(ERegionClassType /*regionClassX*/, ERegionClassType regionClassY) {
        return TRegionClass::Any == regionClassY;
    }
} // NLingBoost
