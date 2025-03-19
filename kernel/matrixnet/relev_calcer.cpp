#include "relev_calcer.h"

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
#include "sliced_util.h"

#include <kernel/factor_storage/factor_storage.h>

#include <util/string/builder.h>

namespace NMatrixnet {

void IRelevCalcer::DoSlicedCalcRelevs(const TFactorStorage* const* features, double* relevs, size_t numDocs) const {
    if (Y_UNLIKELY(features == nullptr || numDocs < 1)) {
        return;
    }

    const size_t numFeats = GetNumFeats();
    size_t totalFeatures = 0;
    for (size_t i = 0; i < numDocs; ++i) {
        totalFeatures += (features[i]->Size() < numFeats ? numFeats : 0);
    }

    TVector<float> extFeaturesHolder;
    extFeaturesHolder.reserve(totalFeatures);
    TVector<const float*> finalFeatures(numDocs, nullptr);
    for (size_t i = 0; i < numDocs; ++i) {
        const auto& view = features[i]->CreateConstView();
        TArrayRef<const float> region{~view, +view};
        finalFeatures[i] = GetOrCopyFeatures(region, numFeats, extFeaturesHolder);
    }

    DoCalcRelevs(&finalFeatures[0], relevs, numDocs);
}

TString IRelevCalcer::GetId() const {
    if (GetInfo() == nullptr) {
        return "_unsetid";
    }

    auto fnamePtr = GetInfo()->FindPtr(TStringBuf("fname"));
    if (auto idPtr = GetInfo()->FindPtr(TStringBuf("formula-id"))) {
        if (fnamePtr != nullptr) {
            return TString::Join(*idPtr, TStringBuf("[") , *fnamePtr,  TStringBuf("]"));
        } else {
            return *idPtr;
        }
    } else if (fnamePtr != nullptr) {
        return TString::Join(TStringBuf("[") , *fnamePtr,  TStringBuf("]"));
    } else {
        return "_unsetid";
    }
}

}   // NMatrixnet
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)
