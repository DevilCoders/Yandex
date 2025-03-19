#pragma once

#include <kernel/ethos/lib/data/dataset.h>

#include <library/cpp/containers/dense_hash/dense_hash.h>

#include <util/generic/set.h>

namespace NEthos {

struct TFeatureStats {
    ui64 FeatureIndex;

    double SumPositiveValues = 0.;
    double SumNegativeValues = 0.;

    double PositiveDiscriminationFactor = 0.;
    double NegativeDiscriminationFactor = 0.;

    void Add(const EBinaryClassLabel label, const TIndexedFloatFeature& feature);
    void SetupDiscriminationFactors(const size_t positivesCount, const size_t negativesCount);
};

class TPositiveDiscriminationLess {
public:
    bool operator () (const TFeatureStats& lhs, const TFeatureStats& rhs) const {
        return lhs.PositiveDiscriminationFactor < rhs.PositiveDiscriminationFactor;
    }
};

class TNegativeDiscriminationLess {
public:
    bool operator () (const TFeatureStats& lhs, const TFeatureStats& rhs) const {
        return lhs.NegativeDiscriminationFactor < rhs.NegativeDiscriminationFactor;
    }
};

template <typename TItem>
class TFeaturesSelector {
private:
    TDenseHash<ui64, TFeatureStats> FeatureStats;

    TDenseHashSet<ui64> SelectedFeatures;

    size_t PositivesCount = 0;
    size_t NegativesCount = 0;
public:
    TFeaturesSelector()
        : FeatureStats((ui64) -1)
        , SelectedFeatures((ui64) -1)
    {
    }

    void Add(const TItem& item, size_t labelIndex = 0) {
        if (!item.HasKnownMark(labelIndex)) {
            return;
        }

        for (const TIndexedFloatFeature feature : item.Features) {
            FeatureStats[feature.Index].Add(item.GetLabel(labelIndex), feature);
        }
        PositivesCount += item.IsPositive(labelIndex);
        NegativesCount += !item.IsPositive(labelIndex);
    }

    void Finish(const size_t featuresToSelectCount) {
        for (auto& featureStats : FeatureStats) {
            featureStats.second.SetupDiscriminationFactors(PositivesCount, NegativesCount);
        }

        TSet<TFeatureStats, TPositiveDiscriminationLess> topPositiveFeatures;
        TSet<TFeatureStats, TNegativeDiscriminationLess> topNegativeFeatures;
        for (auto& featureStats : FeatureStats) {
            ProcessTop(topPositiveFeatures, featureStats.second, featuresToSelectCount);
            ProcessTop(topNegativeFeatures, featureStats.second, featuresToSelectCount);
        }

        SelectedFeatures.MakeEmpty();
        for (auto&& selectedFeature : topNegativeFeatures) {
            SelectedFeatures.Insert(selectedFeature.FeatureIndex);
        }
        for (auto&& selectedFeature : topPositiveFeatures) {
            SelectedFeatures.Insert(selectedFeature.FeatureIndex);
        }
    }

    TItem SelectFeatures(const TItem& origin) const {
        TItem result(origin);
        result.Features.clear();
        for (const TIndexedFloatFeature feature : origin.Features) {
            if (SelectedFeatures.Has(feature.Index)) {
                result.Features.push_back(feature);
            }
        }
        return result;
    }
private:
    template <typename TComparator>
    static void ProcessTop(TSet<TFeatureStats, TComparator>& top,
                            const TFeatureStats& featureStats,
                            const size_t featuresToSelectCount)
    {
        top.insert(featureStats);
        if (top.size() > featuresToSelectCount) {
            top.erase(top.begin());
        }
    }
};

}
