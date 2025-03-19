#include "duplicates_resolvers.h"

namespace NReqBundle {
namespace NDetail {
    const IDuplicatesResolver* TByExpansionTypeDuplicatesResolver::GetResolver(const TExpansionType& type) const {
        return Resolvers.Value(type, DefaultResolver);
    }

    float TByExpansionTypeDuplicatesResolver::GetUpdatedValue(float oldValue, float newValue, const TFacetId& id) const {
        return GetResolver(id.Get<EFacetPartType::Expansion>())->GetUpdatedValue(oldValue, newValue, id);
    }

    float TByExpansionTypeDuplicatesResolver::GetNormalizedValue(float value, const TFacetId& id) const {
        return GetResolver(id.Get<EFacetPartType::Expansion>())->GetNormalizedValue(value, id);
    }
}
}
