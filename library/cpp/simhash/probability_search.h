#pragma once

#include <util/system/defaults.h>
#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/map.h>

i32 ScalarProduct(ui32 n, const i32* a);

double NormalCDF(double x, double mean, double var);

double ProbabilityThatBitFlips(
    double mean,
    double length,
    double lengthDif = 0.2,
    double var = 25000.0);

void ProbabilityThatBitsFlips(
    const i32 simhashRawVector[61],
    ui32 simhashRawVectorLength,
    TVector<double>& resProb,
    double lengthDif = 0.2,
    double var = 25000.0);

class TSimhashMaskEnumerator: private TNonCopyable {
private:
    struct TGenerator {
        TVector<ui32> Flips;

        bool MakeLeftNode(TGenerator& left) const;

        bool MakeRightNode(TGenerator& right) const;
    };

public:
    TSimhashMaskEnumerator(
        const TVector<double>& flipProbabilities,
        ui32 flipCount);

    bool GetNextMask(double& prob, ui64& mask);

private:
    ui64 GetMask(const TGenerator& generator) const;

    double GetProbability(const TGenerator& generator) const;

private:
    TVector<std::pair<double, ui32>> FlipProbabilities;
    TMultiMap<double, TGenerator, TGreater<double>> Generators;
    THashSet<ui64> SeenMasks;
};
