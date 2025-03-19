#pragma once

#include "factor_view.h"

#include <kernel/factor_slices/factor_domain.h>

class TFactorFiller {
public:
    using TFactorWithValue = std::pair<NFactorSlices::TFullFactorIndex, float>;

    TFactorFiller() = default;

    TFactorFiller(const TVector<TFactorWithValue>& factorsToFill)
        : FactorsToFill(factorsToFill)
    {
    }

    void Fill(const TFactorView& view) const {
        for (const auto& factorWithValue : FactorsToFill) {
            if (view.Has(factorWithValue.first)) {
                view[factorWithValue.first] = factorWithValue.second;
            }
        }
    }

private:
    TVector<TFactorWithValue> FactorsToFill;
};
