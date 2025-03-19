#pragma once

#include <kernel/ethos/lib/data/dataset.h>
#include <kernel/ethos/lib/metrics/binary_classification_metrics.h>
#include <kernel/ethos/lib/util/filter_iterator.h>

#include <util/digest/murmur.h>
#include <util/generic/hash.h>
#include <util/thread/pool.h>
#include <utility>

namespace NEthos {

class TQfoldByIndexFilter {
private:
    ui32 FoldCount;
    ui32 ValidationFold = 0;

public:
    TQfoldByIndexFilter(ui32 foldCount)
        : FoldCount(foldCount)
    {
    }

    void SetFold(ui32 fold) {
        ValidationFold = fold;
    }

    template<typename TIndexed>
    bool operator()(const TIndexed& item) const {
        // Based on MatrixNet cross-validation.
        ui32 a[] = {
            item.Index, 0xdeadbeaf, item.Index * 37, item.Index * item.Index,
            0x2b8d7cb4 + 12489, 0x3a0b5f99  + 921487
        };

        ui32 hash = MurmurHash(a, sizeof(a), (ui32)0xf1381976);
        return (hash % FoldCount) != ValidationFold;
    }
};

class TQfoldByGroupFilter {
private:
    ui32 FoldCount;
    ui32 ValidationFold = 0;

    const THashMap<ui32, TString>& GroupsByIndices;

public:
    TQfoldByGroupFilter(ui32 foldCount, const THashMap<ui32, TString>& groupsByIndices)
        : FoldCount(foldCount)
        , GroupsByIndices(groupsByIndices)
    {
    }
    void SetFold(ui32 fold) {
        ValidationFold = fold;
    }

    template<typename TIndexed>
    bool operator()(const TIndexed& item) const {
        return (ComputeHash(GroupsByIndices.at(item.Index)) % FoldCount) != ValidationFold;
    }
};

template<typename TPrediction, typename TMetricsCalculator, typename TMetrics>
class TQfoldCrossValidation {
public:
    template <typename TItem, typename TLearner>
    std::pair<TMetrics, TMetrics> CrossValidateByIndex(const TLearner& learner,
                                         const TVector<TItem>& items,
                                         const ui32 foldCount,
                                         size_t threadsCount)
    {
        TQfoldByIndexFilter filter(foldCount);
        return CrossValidateWithFilter(learner, items, foldCount, filter, threadsCount);
    }

    template <typename TItem, typename TLearner>
    std::pair<TMetrics, TMetrics> CrossValidateByGroup(const TLearner& learner,
                                         const TVector<TItem>& items,
                                         const ui32 foldCount,
                                         const THashMap<ui32, TString>& groupsByIndices,
                                         size_t threadsCount)
    {
        TQfoldByGroupFilter filter(foldCount, groupsByIndices);
        return CrossValidateWithFilter(learner, items, foldCount, filter, threadsCount);
    }
private:
    template<typename TItem, typename TLearner, typename TFilter>
    std::pair<TMetrics, TMetrics> CrossValidateWithFilter(const TLearner& learner,
                                            const TVector<TItem>& items,
                                            const ui32 foldCount,
                                            TFilter& filter,
                                            size_t threadsCount)
    {
        TVector<TMetrics> foldLearnMetrics(foldCount);
        TVector<TMetrics> foldTestMetrics(foldCount);

        THolder<IThreadPool> queue(CreateThreadPool(threadsCount));
        for (ui32 fold = 0; fold < foldCount; ++fold) {
            queue->SafeAddFunc([&, fold](){
                TMetricsCalculator learnMetricsCalculator;
                TMetricsCalculator testMetricsCalculator;

                TFilter filterCopy(filter);
                filterCopy.SetFold(fold);

                using TIterator = TFilterConstIterator<typename TVector<TItem>::const_iterator,
                                                       TItem,
                                                       TFilter>;

                TIterator learnIterator(items.begin(), items.end(), filterCopy);

                typename TLearner::TModel model = learner.Learn(learnIterator, learnIterator.End());

                for (const TItem& item : items) {
                    TPrediction prediction = model.Apply(item);
                    if (filterCopy(item)) {
                        learnMetricsCalculator.Add(prediction, item.Label);
                    } else {
                        testMetricsCalculator.Add(prediction, item.Label);
                    }
                }

                foldLearnMetrics[fold] = learnMetricsCalculator.AllMetrics();
                foldTestMetrics[fold] = testMetricsCalculator.AllMetrics();
            });
        }
        queue->Stop();

        TMetrics learnMetrics, testMetrics;
        for (size_t fold = 0; fold != foldCount; ++fold) {
            learnMetrics.DivideAndAdd(foldLearnMetrics[fold], foldCount);
            testMetrics.DivideAndAdd(foldTestMetrics[fold], foldCount);
        }

        return std::make_pair(learnMetrics, testMetrics);

    }
};

using TBinaryClassificationQFoldCV = TQfoldCrossValidation<TBinaryLabelWithFloatPrediction,
                                                           TBcMetricsCalculator,
                                                           TBcMetrics>;

}
