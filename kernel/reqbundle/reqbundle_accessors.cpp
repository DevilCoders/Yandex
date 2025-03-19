#include "reqbundle_accessors.h"

#include "sequence_accessors.h"
#include "size_limits.h"

namespace NReqBundle {
    namespace NDetail {
        EValidType  IsValidReqBundle(TConstReqBundleAcc bundle, bool needOriginal) {
            TValidConstraints constr;
            constr.NeedOriginal = needOriginal;
            return IsValidReqBundle(bundle, constr);
        }

        EValidType IsValidReqBundle(TConstReqBundleAcc bundle, const TValidConstraints& constr) {
            return IsValidRequests(bundle, constr)
                && IsValidSequence(bundle.GetSequence(), constr)
                && IsValidConstraints(bundle);
        }

        EValidType IsValidRequests(TConstReqBundleAcc bundle, const TValidConstraints& constr) {
            if (constr.NeedNonEmpty && (0 == bundle.GetNumRequests())) {
                return VT_FALSE;
            }

            bool hasOriginal = false;
            TSet<EExpansionType> foundSingleTypes;

            EValidType res = VT_TRUE;
            for (auto request : bundle.GetRequests()) {
                const EValidType requestRes = IsValidRequest(request, bundle.GetSequence());
                if (VT_FALSE == requestRes) {
                    return VT_FALSE;
                }
                res = res && requestRes;

                for (auto entry : request.GetFacets().GetEntries()) {
                    const EExpansionType expansionType = entry.GetId().Get<EFacetPartType::Expansion>();
                    if (!GetExpansionTraitsByType(expansionType).IsMultiRequest) {
                        if (foundSingleTypes.contains(expansionType)) {
                            return VT_FALSE;
                        } else {
                            foundSingleTypes.insert(expansionType);
                            if (expansionType == TExpansion::OriginalRequest) {
                                hasOriginal = true;
                            }
                        }
                    }
                }
            }

            if (constr.NeedOriginal && !hasOriginal) {
                return VT_FALSE;
            }
            return res;
        }

        EValidType IsValidConstraints(TConstReqBundleAcc bundle) {
            if (bundle.GetNumConstraints() > TSizeLimits::MaxNumConstraints) {
                return VT_FALSE;
            }
            EValidType res = VT_TRUE;
            for (auto constraint : bundle.GetConstraints()) {
                const EValidType constraintRes = IsValidConstraint(constraint, bundle.GetSequence());
                if (VT_FALSE == constraintRes) {
                    return VT_FALSE;
                }
                res = res && constraintRes;
            }
            return res;
        }

        TRequestsRemapper::TRequestsRemapper(size_t numRequests)
            : TVector<size_t>(numRequests, Max<size_t>())
        {}
        void TRequestsRemapper::Reset(size_t numRequests) {
            assign(numRequests, Max<size_t>());
        }

        bool TRequestsRemapper::Has(size_t requestIndex) const {
            return requestIndex < size()
                && (*this)[requestIndex] != Max<size_t>();
        }
    } // NDetail
} // NReqBundle
