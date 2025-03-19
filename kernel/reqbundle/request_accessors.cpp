#include "request_accessors.h"

namespace NReqBundle {
    namespace NDetail {
        EValidType IsValidFacet(TConstFacetEntryAcc entry) {
            const bool ok = IsFullyDefinedFacetId(entry.GetId())
                && (entry.GetExpansion() != TExpansion::OriginalRequest
                    || entry.GetId() == MakeFacetId(TExpansion::OriginalRequest))
                && (entry.GetValue() >= 0.0f && entry.GetValue() <= 1.0f);

            return (ok ? VT_TRUE : VT_FALSE);
        }
    } // NDetail
} // NReqBundle
