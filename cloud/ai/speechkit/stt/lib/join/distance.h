#pragma once

//
// Created by Mikhail Yutman on 29.04.2020.
//

#pragma once

#include "structures.h"

#include <util/generic/array_ref.h>
#include <util/generic/fwd.h>

#include <cstddef>
#include <math.h>
#include <limits>

namespace {
    const double SIGMA = 705.55;
    const double ADDITIONAL = log(1 / (sqrt(2 * M_PI) * SIGMA));
}

template <class T>
class IDistance {
public:
    [[nodiscard]] virtual TVector<TVector<T>> CalculateAllDistancesBackward(const TText& ref1, const TText& ref2) const = 0;

    [[nodiscard]] T CalculateDistance(const TText& sa, const TText& sb) const {
        return CalculateAllDistancesBackward(sa, sb)[0][0];
    }

    [[nodiscard]] TVector<T> CalculateAllDistancesBackwardTemp(const TText& temp, const TText& ref) const {
        return CalculateAllDistancesBackward(temp, ref)[0];
    }

    [[nodiscard]] virtual TVector<double> RestoreLikelihoods(const TText& temp, const TText& ref) const {
        Y_UNUSED(temp);
        Y_UNUSED(ref);
        return {};
    }

    static T Max() {
        return std::numeric_limits<T>::max();
    }
};

class TLevenshteinDistance: public IDistance<ui32> {
public:
    [[nodiscard]] TVector<TVector<ui32>> CalculateAllDistancesBackward(const TText& ref1, const TText& ref2) const override;
};

class TLikelihoodLevensteinDistance: public IDistance<double> {
private:
    const double Weight;

public:
    explicit TLikelihoodLevensteinDistance(double weight = 0)
        : Weight(weight)
    {
    }

    [[nodiscard]] TVector<TVector<double>> CalculateAllDistancesBackward(const TText& ref1, const TText& ref2) const override;

    [[nodiscard]] TVector<double> RestoreLikelihoods(const TText& temp, const TText& ref) const override;
};

class TWordwiseLevenshteinDistance: public IDistance<ui32> {
public:
    [[nodiscard]] TVector<TVector<ui32>> CalculateAllDistancesBackward(const TText& ref1, const TText& ref2) const override;
};

class TLikelihoodWordwiseLevensteinDistance: public IDistance<double> {
private:
    const double Weight;

public:
    explicit TLikelihoodWordwiseLevensteinDistance(double weight = 0)
        : Weight(weight)
    {
    }

    [[nodiscard]] TVector<TVector<double>> CalculateAllDistancesBackward(const TText& ref1, const TText& ref2) const override;

    [[nodiscard]] TVector<double> RestoreLikelihoods(const TText& temp, const TText& ref) const override;
};

class TTupleLikelihoodLevensteinDistance: public IDistance<TLikelihoodValue> {
private:
    const double Weight;

public:
    explicit TTupleLikelihoodLevensteinDistance(double weight = 0)
        : Weight(-weight)
    {
    }

    [[nodiscard]] TVector<TVector<TLikelihoodValue>> CalculateAllDistancesBackward(const TText& ref1, const TText& ref2) const override;

    [[nodiscard]] TVector<double> RestoreLikelihoods(const TText& temp, const TText& ref) const override;
};

class TTupleLikelihoodWordwiseLevensteinDistance: public IDistance<TLikelihoodValue> {
private:
    const double Weight;

public:
    explicit TTupleLikelihoodWordwiseLevensteinDistance(double weight = 0)
        : Weight(-weight)
    {
    }

    [[nodiscard]] TVector<TVector<TLikelihoodValue>> CalculateAllDistancesBackward(const TText& ref1, const TText& ref2) const override;

    [[nodiscard]] TVector<double> RestoreLikelihoods(const TText& temp, const TText& ref) const override;
};

template <class T>
TText UniteBitMarkups(const IDistance<T>& distance, TConstArrayRef<TText> vars) {
    TVector<TVector<T>> dst(vars.size(), TVector<T>(vars.size()));
    T minDist = distance.Max();
    TText ans;
    for (int i = 0; i < vars.ysize(); i++) {
        for (int j = i + 1; j < vars.ysize(); j++) {
            dst[i][j] = dst[j][i] = distance.CalculateDistance(vars[i], vars[j]);
        }
        T sum = 0;
        for (int j = 0; j < vars.ysize(); j++) {
            sum += dst[i][j];
        }
        if (minDist > sum) {
            minDist = sum;
            ans = vars[i];
        }
    }
    return ans;
}
