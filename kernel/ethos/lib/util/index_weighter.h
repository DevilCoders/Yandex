#pragma once

#include <util/generic/vector.h>
#include <util/ysaveload.h>

namespace NEthos {

class TIndexWeighter {
private:
    size_t Threshold = 500;
    double Factor = 0.05;

    TVector<double> PrecalculatedWeights;

public:
    Y_SAVELOAD_DEFINE(Threshold, Factor, PrecalculatedWeights);

    TIndexWeighter() {
        Update();
    }

    TIndexWeighter(size_t threshold, double factor)
        : Threshold(threshold)
        , Factor(factor)
    {
        Update();
    }

    inline double operator()(size_t wordPosition) const {
        return wordPosition < PrecalculatedWeights.size() ? PrecalculatedWeights[wordPosition]
                                                          : PrecalculatedWeights.back();
    }

    void SetThreshold(size_t threshold) {
        Threshold = threshold;
        Update();
    }

    void SetFactor(double factor) {
        Factor = factor;
        Update();
    }
private:
    double CalculateWeight(float index) const;

    void Update();
};

}
