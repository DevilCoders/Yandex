#include <kernel/factor_storage/factor_storage.h>
#include <kernel/matrixnet/mn_sse.h>

#include <util/system/yassert.h>
#include <util/generic/vector.h>
#include <util/generic/algorithm.h>
#include <util/generic/mapfindptr.h>

#include "relev_fml.h"

namespace NRelevFml {

namespace {

typedef TVector<TFactorStorage> TSamples;

inline bool AddCornerValue(const TFactorStorage& factorStorage, const TFullFactorIndex& factorIndex, const float border, const bool left, TSamples& samples) {
    if (left && factorStorage[factorIndex] <= border || !left && factorStorage[factorIndex] > border) {
        return false;
    }

    TFactorStorage y(factorStorage);
    y[factorIndex] = border + (left ? -0.1f : 0.1f);
    samples.push_back(std::move(y));

    return true;
}

bool CreateSmartSamples(const TFullFactorIndex& factorIndex, NMatrixnet::TBorder& border, const TFactorStorage& factorStorage, TSamples& samples) {
    Sort(border.begin(), border.end());

    bool realTouched = !AddCornerValue(factorStorage, factorIndex, border[0], true, samples);

    for (size_t i = 1; i < border.size(); ++i) {
        if (factorStorage[factorIndex] > border[i - 1] && factorStorage[factorIndex] <= border[i]) {
            if (realTouched) {
                return false;
            }

            realTouched = true;
            continue;
        }

        TFactorStorage y(factorStorage);
        y[factorIndex] = (border[i - 1] + border[i]) / 2;
        samples.push_back(std::move(y));
    }

    if (!AddCornerValue(factorStorage, factorIndex, border.back(), false, samples) && realTouched) {
        return false;
    }

    return true;
}

void CreateUniformSamples(const TFullFactorIndex& factor, const int stepCount, const TFactorStorage& factorStorage, TSamples& samples) {
    TOneFactorInfo info = factorStorage.GetDomain().GetFactorInfo(factor.Index, factor.Slice);
    if (!info) {
        return;
    }
    if (info.IsBinary()) {
        {
            TFactorStorage y(factorStorage);
            y[factor] = 0;
            samples.push_back(std::move(y));
        }

        {
            TFactorStorage y(factorStorage);
            y[factor] = 1;
            samples.push_back(std::move(y));
        }
    } else {
        for (int i = 0; i < stepCount; ++i) {
            if (fabs(static_cast<float>(i) / stepCount - factorStorage[factor]) < 1.0f / stepCount) {
                continue;
            }

            TFactorStorage y(factorStorage);
            y[factor] = static_cast<float>(i) / stepCount;
            samples.push_back(std::move(y));
        }
    }
}

bool CreateSamples(
        const NMatrixnet::TFactorsMap& factorsMap,
        NMatrixnet::TBorders& borders,
        const int stepCount,
        const TFactorStorage& factorStorage,
        TSamples& samples,
        TVector<size_t>& offsets)
{
    samples.clear();
    samples.reserve(borders.size() * (stepCount < 1 ? 32 : stepCount));
    offsets.clear();
    offsets.reserve(borders.size());

    for (auto& x : borders) {
        if (x.first < factorStorage.Size()) {
            offsets.push_back(samples.size());
            if (x.second.empty()) {
                continue;
            }

            const TFullFactorIndex* factor = MapFindPtr(factorsMap, x.first);
            if (!factor) {
                return false;
            }

            if (stepCount < 1 && !CreateSmartSamples(*factor, x.second, factorStorage, samples)) {
                return false;
            }

            if (stepCount > 0) {
                CreateUniformSamples(*factor, stepCount, factorStorage, samples);
            }
        } else {
            return false;
        }
    }

    return true;
}

void CalcDerivative(
        const TVector<double>& res,
        const NMatrixnet::TBorder& border,
        const int stepCount,
        const float factorValue,
        const size_t offset,
        const bool isBinary,
        float& derivePos,
        float& deriveNeg)
{
    derivePos = 0;
    deriveNeg = 0;

    size_t numBorders = (stepCount < 1 ? border.size() : (isBinary ? 2 : stepCount));

    for (size_t i = offset; i < offset + numBorders; ++i) {
        if (stepCount > 0) {
            if (static_cast<float>(i - offset) / stepCount - factorValue < 1.0f / stepCount) {
                --numBorders;
            } else {
                float curDiff = res[i] - res.back();
                float derive = curDiff / (static_cast<float>(i - offset) / stepCount - factorValue);
                derivePos += fabs(derive);
                deriveNeg += derive;
            }
        } else {
            float curDiff = res[i] - res.back();
            derivePos = Max<float>(derivePos, curDiff);
            deriveNeg = Min<float>(deriveNeg, curDiff);
        }
    }
}

}

bool DeriveMatrixnet(
        const NMatrixnet::TMnSseInfo* matrixnet,
        const TFactorStorage& factorStorage,
        TFactorStorage& deriveWebPos,
        TFactorStorage& deriveWebNeg,
        const int stepCount)
{
    Y_ASSERT(matrixnet);
    Y_ASSERT(deriveWebPos.Size() == deriveWebNeg.Size());
    Y_ASSERT(deriveWebPos.Size() == factorStorage.Size());

    NMatrixnet::TFactorsMap factorsMap;
    matrixnet->FactorsMap(factorsMap);

    for (size_t i = 0; i < deriveWebPos.Size(); ++i) {
        deriveWebPos[i] = deriveWebNeg[i] = 0;
    }

    TSamples samples;
    TVector<size_t> offsets;
    NMatrixnet::TBorders borders;

    matrixnet->Borders(borders);

    if (!CreateSamples(factorsMap, borders, stepCount, factorStorage, samples, offsets)) {
        return false;
    }

    samples.push_back(factorStorage);
    TVector<double> res(samples.size());
    TVector<const TFactorStorage*> samplesPtr;
    for (const TFactorStorage& fs : samples) {
        samplesPtr.push_back(&fs);
    }
    matrixnet->DoSlicedCalcRelevs(samplesPtr.data(), res.data(), res.size());

    size_t cnt = 0;
    for (const auto& x : borders) {
        if (x.first < deriveWebPos.Size()) {
            if (const TFullFactorIndex* factor = MapFindPtr(factorsMap, x.first)) {
                TOneFactorInfo info = factorStorage.GetDomain().GetFactorInfo(factor->Index, factor->Slice);
                if (info) {
                    CalcDerivative(res, x.second, stepCount, factorStorage[*factor], offsets[cnt++], info.IsBinary(), deriveWebPos[*factor], deriveWebNeg[*factor]);
                }
            }
        }
    }

    return true;
}

} // namespace NRelevFml
