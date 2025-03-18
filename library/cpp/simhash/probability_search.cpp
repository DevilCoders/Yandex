#include "probability_search.h"

#include "randoms.h"

#include <util/generic/algorithm.h>
#include <util/generic/singleton.h>

#include <math.h>

i32 ScalarProduct(ui32 n, const i32* a) {
    static const TRandomVectors& Vectors = RandomVectorsFactory().GetRandomVectors(61);
    i32 res = 0;
    for (int i = 0; i < 61; ++i) {
        res += a[i] * Vectors.Vectors()(n, i);
    }
    return res;
}

double NormalCDF(double x, double mean, double var) {
    return 0.5 * (1.0 + erf((x - mean) / sqrt(2.0 * var)));
}

double ProbabilityThatBitFlips(
    double mean,
    double length,
    double lengthDif,
    double var) {
    if (length == 0) {
        return 0.0;
    }
    return NormalCDF(0, fabs(mean), var * length * lengthDif);
}

void ProbabilityThatBitsFlips(
    const i32 simhashRawVector[61],
    ui32 simhashRawVectorLength,
    TVector<double>& resProb,
    double lengthDif,
    double var) {
    resProb.clear();
    resProb.resize(64);
    for (size_t i = 0; i < 64; ++i) {
        resProb[i] = ProbabilityThatBitFlips(ScalarProduct(i, simhashRawVector),
                                             simhashRawVectorLength,
                                             lengthDif, var);
    }
}

bool TSimhashMaskEnumerator::TGenerator::MakeLeftNode(TGenerator& left) const {
    if (Flips.size() == 0) {
        ythrow yexception() << "Flips.size() == 0";
    }
    if (Flips.back() == 63) {
        return false;
    }
    left.Flips.assign(Flips.begin(), Flips.end());
    left.Flips.back() += 1;
    return true;
}

bool TSimhashMaskEnumerator::TGenerator::MakeRightNode(TGenerator& right) const {
    if (Flips.size() == 0) {
        ythrow yexception() << "Flips.size() == 0";
    }
    for (ui32 i = Flips.size() - 1; i >= 1; --i) {
        if (Flips[i] <= Flips[i - 1]) {
            ythrow yexception() << "Flips[i] <= Flips[i - 1]";
        }
        if (Flips[i] - Flips[i - 1] == 2) {
            if (Flips[i - 1] == 63) {
                return false;
            }
            right.Flips.assign(Flips.begin(), Flips.end());
            right.Flips[i - 1] += 1;
            return true;
        } else if (Flips[i] - Flips[i - 1] > 2) {
            return false;
        }
    }
    return false;
}

TSimhashMaskEnumerator::TSimhashMaskEnumerator(
    const TVector<double>& flipProbabilities,
    ui32 flipCount)
    : FlipProbabilities()
    , Generators()
    , SeenMasks()
{
    if (flipCount == 0) {
        ythrow yexception() << "flipCount == 0";
    }
    if (flipProbabilities.size() != 64) {
        ythrow yexception() << "flipProbabilities.size() != 64";
    }
    for (ui32 i = 0; i < flipProbabilities.size(); ++i) {
        if (flipProbabilities[i] > 1.0) {
            ythrow yexception() << "flipProbabilities[i] > 1.0";
        }
        FlipProbabilities.push_back(std::make_pair(flipProbabilities[i], i));
    }
    ::Sort(FlipProbabilities, TGreater<std::pair<double, ui32>>());

    for (ui32 i = 1; i <= flipCount; ++i) {
        TGenerator generator;
        for (ui32 j = 0; j < i; ++j) {
            generator.Flips.push_back(j);
        }
        double prob = GetProbability(generator);
        SeenMasks.insert(GetMask(generator));
        if (prob > 0) {
            Generators.insert(std::make_pair(prob, generator));
        }
    }
}

bool TSimhashMaskEnumerator::GetNextMask(double& prob1, ui64& mask1) {
    if (Generators.empty()) {
        return false;
    }
    const std::pair<double, TGenerator>& max = *Generators.cbegin();
    prob1 = max.first;
    if (prob1 == 0.0) {
        ythrow yexception() << "prob == 0.0";
    }
    mask1 = GetMask(max.second);

    TGenerator left;
    TGenerator right;
    bool hasLeft = max.second.MakeLeftNode(left);
    bool hasRight = max.second.MakeRightNode(right);

    Generators.erase(Generators.begin());

    if (hasLeft) {
        ui64 mask2 = GetMask(left);
        if (!SeenMasks.contains(mask2)) {
            SeenMasks.insert(mask2);
            double prob2 = GetProbability(left);
            if (prob2 > 0.0) {
                Generators.insert(std::make_pair(prob2, left));
            }
        }
    }
    if (hasRight) {
        ui64 mask3 = GetMask(right);
        if (!SeenMasks.contains(mask3)) {
            SeenMasks.insert(mask3);
            double prob3 = GetProbability(right);
            if (prob3 > 0.0) {
                Generators.insert(std::make_pair(prob3, right));
            }
        }
    }

    return true;
}

ui64 TSimhashMaskEnumerator::GetMask(const TGenerator& generator) const {
    ui64 res = 0;
    if (generator.Flips.size() == 0) {
        ythrow yexception() << "Flips.size() == 0";
    }
    for (ui32 i = 0; i < generator.Flips.size(); ++i) {
        res |= (((ui64)1) << FlipProbabilities[generator.Flips[i]].second);
    }
    return res;
}

double TSimhashMaskEnumerator::GetProbability(const TGenerator& generator) const {
    double res = 1.0;
    for (ui32 i = 0; i < 64; ++i) {
        if (::Find(generator.Flips.begin(), generator.Flips.end(), i) == generator.Flips.end()) {
            if (FlipProbabilities[i].first == 0.0) {
                break;
            }
            res *= (1.0 - FlipProbabilities[i].first);
        } else {
            res *= FlipProbabilities[i].first;
        }
    }
    return res;
}
