#pragma once

#include "reqbundle_fwd.h"

#include <kernel/text_machine/structured_id/full_id.h>
#include <kernel/text_machine/structured_id/typed_param.h>
#include <kernel/lingboost/constants.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/deque.h>

namespace NReqBundle {
    enum class EFacetPartType {
        Expansion = 0,
        RegionId = 1,
        FacetPartMax
    };

    namespace NDetail {
        struct TFacetIdBuilder : public ::NStructuredId::TIdBuilder<EFacetPartType> {
            using TFacetPartsList = TListType<
                TPair<EFacetPartType::Expansion, ::NStructuredId::TPartMixin<EExpansionType>>,
                TPair<EFacetPartType::RegionId, ::NStructuredId::TPartMixin<TRegionId>>
            >;
        };
    } // NDetail
} // NReqBundle

namespace NStructuredId {
    template <>
    struct TGetIdTraitsList<NReqBundle::EFacetPartType> {
        using TResult = NReqBundle::NDetail::TFacetIdBuilder::TFacetPartsList;
    };
} // NStructuredId

namespace NReqBundle {
    using TFacetId = ::NStructuredId::TFullId<EFacetPartType>;

    inline bool IsFullyDefinedFacetId(const TFacetId& id) {
        return id.IsValid<EFacetPartType::Expansion>() && id.IsValid<EFacetPartType::RegionId>();
    }

    template <typename... Args>
    inline TFacetId MakeFacetId(Args&&... args) {
        static TFacetId baseId(
            TExpansion::OriginalRequest,
            TRegionId::World()
        );

        TFacetId res = baseId;
        res.Set(std::forward<Args>(args)...);
        return res;
    }

    template <typename... Args>
    inline TFacetId MakeFacetIdPredicate(Args&&... args) {
        return TFacetId(std::forward<Args>(args)...);
    }

    TFacetId FacetIdFromJson(const NJson::TJsonValue& value);
    NJson::TJsonValue FacetIdToJson(const TFacetId& facetId);

    struct TIsSubRegion
        : public NStructuredId::NDetail::TIsEqual
    {
        using NStructuredId::NDetail::TIsEqual::IsSubValue;

        inline bool IsSubValue(const TRegionId& x, const TRegionId& y) {
            return x == y
                || NLingBoost::GetRegionClassByRegionId(x) == NLingBoost::DecodeRegionClass(y);
        }
    };
} // NReqBundle

namespace NReqBundle {
namespace NDetail {
    struct TFacetEntryData {
        TFacetId Id;
        float Value = 1.0f;
    };

    struct TFacetsData {
        using TEntry = TFacetEntryData;
        TDeque<TEntry> Entries; // should always be sorted by TEntry::Id
    };

    inline bool operator < (const TFacetsData::TEntry& x, const TFacetsData::TEntry& y) {
        return x.Id < y.Id;
    }

    using TInsertFacetResult = std::pair<TFacetsData::TEntry&, bool>;

    TFacetsData::TEntry* LookupFacet(TFacetsData& data, const TFacetId& id); // log(N)
    const TFacetsData::TEntry* LookupFacet(const TFacetsData& data, const TFacetId& id); // log(N)
    TInsertFacetResult InsertFacet(TFacetsData& data, const TFacetId& id); // N
    void RemoveFacet(TFacetsData& data, const TFacetId& id); // N
} // NDetail
} // NReqBundle

bool FromString(const TStringBuf& text, NReqBundle::EFacetPartType& value);
