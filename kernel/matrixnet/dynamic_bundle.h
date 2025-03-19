#pragma once

#include <kernel/factor_slices/factor_slices.h>
#include <kernel/matrixnet/relev_calcer.h>

#include <util/generic/vector.h>
#include <util/stream/input.h>
#include <util/stream/output.h>

namespace NMatrixnet {

/* Tells that there is need to renorm result on [TreeIndexFrom; TreeIndexTo)
 * add Bias then multiply by features[FeatureIndex]
 */
struct TDynamicBundleComponent {
    size_t TreeIndexFrom = 0;
    size_t TreeIndexTo = 0;
    NFactorSlices::TFullFactorIndex FeatureIndex;
    double Bias = 0.0;
    double Scale = 1.0;
    TString FormulaId;

    bool operator==(const TDynamicBundleComponent& other) const;
};

struct TDynamicBundle {
    TVector<TDynamicBundleComponent> Components;

    void Load(IInputStream* in);
    void Save(IOutputStream* out) const;

    static inline constexpr char InfoKeyValue[] = "DynamicBundle";
    static bool IsDynamicBundle(const IRelevCalcer& calcer) {
        return calcer.GetInfo()->contains(TDynamicBundle::InfoKeyValue);
    }
};

}
