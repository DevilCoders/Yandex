#include "facets.h"

#include <kernel/country_data/countries.h>

#include <util/generic/algorithm.h>

using namespace NReqBundle;
using namespace NReqBundle::NDetail;

namespace {
    TFacetsData::TEntry EntryById(const TFacetId& id) {
        TFacetsData::TEntry entry;
        entry.Id = id;
        return entry;
    }
}

namespace NReqBundle {
    TFacetId FacetIdFromJson(const NJson::TJsonValue& value) {
        Y_ENSURE(
            value.IsMap(),
            "facet value is not a map, " << value
        );

        TFacetId::TStringId strId;

        for (auto& entry : value.GetMapSafe()) {
            EFacetPartType partType = EFacetPartType::FacetPartMax;
            Y_ENSURE(FromString(entry.first, partType), "unknown facet part: " << entry.first);

            if (EFacetPartType::RegionId == partType) {
                TRegionId regionId;
                ERegionClassType regionClass = TRegionClass::RegionClassMax;
                if (entry.second.IsInteger()) {
                    strId[partType] = ToString(entry.second.GetIntegerSafe());
                } else if (TryFromString(entry.second.GetStringSafe(), regionId)) {
                    strId[partType] = entry.second.GetStringSafe();
                } else if (FromString(entry.second.GetStringSafe(), regionClass)) {
                    strId[partType] = ToString(NLingBoost::EncodeRegionClass(regionClass));
                } else {
                    ythrow yexception() << "failed to parse region string, "
                        << entry.second.GetStringSafe();
                }
            } else {
                strId[partType] = entry.second.GetStringSafe();
            }
        }

        return FromStringId(strId);
    }

    NJson::TJsonValue FacetIdToJson(const TFacetId& id) {
        TFacetId::TStringId strId = ToStringId(id);

        NJson::TJsonValue value{NJson::JSON_MAP};
        for (auto& entry : strId) {
            value[ToString(entry.first)] = entry.second;
        }

        return value;
    }
} // NReqBundle

namespace NReqBundle {
namespace NDetail {
    TFacetsData::TEntry* LookupFacet(TFacetsData& data, const TFacetId& id) {
        Y_ASSERT(IsFullyDefinedFacetId(id));

        auto range = EqualRange(data.Entries.begin(), data.Entries.end(), EntryById(id));
        Y_ASSERT(range.second == range.first || range.second - range.first == 1);
        if (range.second > range.first) {
            return &(*range.first);
        }
        return nullptr;
    }

    const TFacetsData::TEntry* LookupFacet(const TFacetsData& data, const TFacetId& id) {
        return LookupFacet(const_cast<TFacetsData&>(data), id);
    }

    TInsertFacetResult InsertFacet(TFacetsData& data, const TFacetId& id) {
        Y_ASSERT(IsFullyDefinedFacetId(id));

        auto pos = LowerBound(data.Entries.begin(), data.Entries.end(), EntryById(id));
        if (pos != data.Entries.end() && pos->Id == id) {
            return TInsertFacetResult(*pos, false);
        }
        auto& entry = *data.Entries.emplace(pos);
        entry.Id = id;
        return TInsertFacetResult(entry, true);
    }

    void RemoveFacet(TFacetsData& data, const TFacetId& id) {
        Y_ASSERT(IsFullyDefinedFacetId(id));

        auto range = EqualRange(data.Entries.begin(), data.Entries.end(), EntryById(id));
        Y_ASSERT(range.second == range.first || range.second - range.first == 1);
        data.Entries.erase(range.first, range.second);
    }
} // NDetail
} // NReqBundle
