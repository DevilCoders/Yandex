#pragma once

#include "linear_regression.h"
#include "options.h"
#include "pool.h"
#include "splitter.h"

#include <util/generic/ylimits.h>
#include <util/thread/pool.h>
#include <util/ysaveload.h>

namespace NRegTree {

template <typename TFloatType>
struct TTreeNodePruneInfo {
    size_t SamplesCount;

    TFloatType SelfSumSquaredErrors;
    size_t SelfParametersCount;

    TFloatType SubtreeSumSquaredErrors;
    size_t SubtreeParametersCount;

    bool Pruned;

    TTreeNodePruneInfo()
        : SamplesCount(0)
        , SelfSumSquaredErrors(TFloatType())
        , SelfParametersCount(0)
        , SubtreeSumSquaredErrors(TFloatType())
        , SubtreeParametersCount(0)
        , Pruned(false)
    {
    }

    TTreeNodePruneInfo(const TLinearModel<TFloatType>& linearModel, TFloatType sumSquaredErrors, size_t samplesCount = 0)
        : SamplesCount(samplesCount)
        , SelfSumSquaredErrors(sumSquaredErrors)
        , SelfParametersCount(0)
        , SubtreeSumSquaredErrors(sumSquaredErrors)
        , SubtreeParametersCount(0)
        , Pruned(false)
    {
        for (const TFloatType* coefficient = linearModel.Coefficients.begin(); coefficient != linearModel.Coefficients.end(); ++coefficient) {
            if (*coefficient) {
                ++SelfParametersCount;
            }
        }
        SubtreeParametersCount = SelfParametersCount;
    }

    static float PruneScore(size_t parametersCount,
                            size_t samplesCount,
                            TFloatType sumSquaredErrors,
                            TFloatType pruneFactor)
    {
        if (samplesCount <= pruneFactor * parametersCount) {
            return sumSquaredErrors * 1000000 + 1e25;
        }

        return sumSquaredErrors * (samplesCount + pruneFactor * parametersCount) / (samplesCount - pruneFactor * parametersCount);
    }

    float SelfPruneScore(TFloatType pruneFactor) const {
        return PruneScore(SelfParametersCount, SamplesCount, SelfSumSquaredErrors, pruneFactor);
    }

    float SubtreePruneScore(TFloatType pruneFactor) const {
        return PruneScore(SubtreeParametersCount, SamplesCount, SubtreeSumSquaredErrors, pruneFactor);
    }

    bool NeedsPruning(TFloatType pruneFactor) const {
        return SelfPruneScore(pruneFactor) < SubtreePruneScore(pruneFactor);
    }

    void SetChildren(const TTreeNodePruneInfo<TFloatType>& leftChild, const TTreeNodePruneInfo<TFloatType>& rightChild) {
        Y_ASSERT(SamplesCount == leftChild.SamplesCount + rightChild.SamplesCount);

        SubtreeParametersCount = leftChild.SubtreeParametersCount + rightChild.SubtreeParametersCount;
        SubtreeSumSquaredErrors = leftChild.SubtreeSumSquaredErrors + rightChild.SubtreeSumSquaredErrors;
    }

    void Prune() {
        SubtreeParametersCount = SelfParametersCount;
        SubtreeSumSquaredErrors = SelfSumSquaredErrors;
        Pruned = true;
    }
};

}
