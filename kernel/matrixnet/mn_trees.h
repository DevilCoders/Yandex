#pragma once

#include <util/system/defaults.h>
#include <util/generic/vector.h>

#include "model_info.h"
#include "relev_calcer.h"

namespace NMatrixnet {

/// Simple storage for matrixnet model. For production calculations use TMnSse.
class TMnTree: public IRelevCalcer {
public:
    TMnTree() {}

    explicit TMnTree(const int size);

    void SetCondition(const int idx, const ui32 featureId, const float border);

    void SetValues(const double* values);

    int CalcValueIndex(const float* features) const {
        int idx = 0;
        for (int i = 0; i < Features.ysize(); ++i) {
            idx |= (features[Features[i]] > Borders[i]) << i;
        }

        return idx;
    }

    const TVector<ui32>& GetFeatures() const {
        return Features;
    }

    const TVector<float>& GetBorders() const {
        return Borders;
    }

    const TVector<double>& GetValues() const {
        return Values;
    }

public:
    // from IRelevCalcer
    size_t GetNumFeats() const override;
    double DoCalcRelev(const float* features) const override {
        return Values[CalcValueIndex(features)];
    }

private:
    /// feature ids in conditions
    TVector<ui32>  Features;

    /// condition values: plane[features[i]] > borders[i]
    TVector<float> Borders;

    /// 2^n values for each tree leaf, where n is size of features/borders vector
    TVector<double> Values;
};

struct TMnTrees: public IRelevCalcer {
    /// model value is sum of tree values
    TVector<TMnTree> Trees;

    /// the result is summation of all trees plus this Bias
    double Bias;

    TModelInfo Info;

public:
    // from IRelevCalcer
    size_t GetNumFeats() const override;
    double DoCalcRelev(const float* features) const override;
    const NMatrixnet::TModelInfo* GetInfo() const override {
        return &Info;
    }
};

}

