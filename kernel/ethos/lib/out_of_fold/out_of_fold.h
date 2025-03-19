#pragma once

#include <kernel/ethos/lib/data/dataset.h>
#include <kernel/ethos/lib/data/linear_model.h>
#include <kernel/ethos/lib/linear_classifier_options/options.h>
#include <kernel/ethos/lib/util/any_iterator.h>
#include <kernel/ethos/lib/util/shuffle_iterator.h>

#include <util/datetime/cputimer.h>
#include <util/generic/hash.h>
#include <util/generic/array_ref.h>
#include <util/random/mersenne.h>
#include <util/random/shuffle.h>
#include <util/string/printf.h>
#include <util/system/mutex.h>
#include <util/thread/pool.h>

namespace NEthos {

template <typename TLearnerImpl>
TFloatFeatureWeightMap LearnGrowingFoldFeatures(const TLinearClassifierOptions& linearClassifierOptions,
                                                TAnyIterator<TBinaryLabelFloatFeatureVector> begin,
                                                TAnyIterator<TBinaryLabelFloatFeatureVector> end,
                                                const size_t featureNumber)
{
    TMersenne<ui64> mersenne;

    TShuffleIterator<TBinaryLabelFloatFeatureVector> shuffleIterator(begin, end, mersenne);

    TShuffleIterator<TBinaryLabelFloatFeatureVector> shuffleBegin = shuffleIterator.Begin();
    TShuffleIterator<TBinaryLabelFloatFeatureVector> shuffleEnd = shuffleIterator.End();

    TFloatFeatureWeightMap weights;

    size_t limit = 0;
    for (;;) {
        const size_t newLimit = limit ? limit * 2 : 1;

        TShuffleIterator<TBinaryLabelFloatFeatureVector> newElementsBegin = shuffleIterator.Iterator(limit);
        TShuffleIterator<TBinaryLabelFloatFeatureVector> newElementsEnd = shuffleIterator.Iterator(newLimit);

        for (; newElementsBegin != newElementsEnd; ++newElementsBegin) {
            newElementsBegin->ExternalFeatures[featureNumber] = LinearPrediction(weights, newElementsBegin->Features);
        }

        weights.Clear();
        TLearnerImpl impl(linearClassifierOptions);
        impl.Learn(shuffleBegin, newElementsEnd, 0, &weights);

        limit = newLimit;
        if (newElementsEnd == shuffleEnd) {
            break;
        }
    }
    return weights;
}

template <typename TLearnerImpl>
TFloatFeatureWeightMap LearnOutOfFoldFeatures(const TLinearClassifierOptions& linearClassifierOptions,
                                              TAnyIterator<TBinaryLabelFloatFeatureVector> begin,
                                              TAnyIterator<TBinaryLabelFloatFeatureVector> end,
                                              const size_t featureNumber)
{
    TMersenne<ui64> mersenne;
    TShuffleIterator<TBinaryLabelFloatFeatureVector> shuffleIterator(begin, end, mersenne);

    TLearnerImpl impl(linearClassifierOptions);

    const size_t foldsCount = linearClassifierOptions.FeatureFoldsCount;
    for (size_t foldNumber = 0; foldNumber < foldsCount; ++foldNumber) {
        TShuffleIterator<TBinaryLabelFloatFeatureVector> learnIterator = shuffleIterator.SkipIterator(foldNumber, foldsCount);

        TFloatFeatureWeightMap weights;
        impl.Learn(learnIterator.Begin(), learnIterator.End(), 0, &weights);

        TShuffleIterator<TBinaryLabelFloatFeatureVector> featuresIterator = shuffleIterator.FoldIterator(foldNumber, foldsCount);
        for (; featuresIterator.IsValid(); ++featuresIterator) {
            featuresIterator->ExternalFeatures[featureNumber] = LinearPrediction(weights, featuresIterator->Features);
        }
    }

    TFloatFeatureWeightMap weights;
    impl.Learn(begin, end, 0, &weights);
    return weights;
}

}
