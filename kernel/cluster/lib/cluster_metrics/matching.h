#pragma once

#include <util/system/defaults.h>
#include <util/system/yassert.h>

namespace NClusterMetrics {

template <typename TClusterId>
class TMatching {
private:
    TClusterId SampleClusterId;

    ui32 SampleClusterSize;
    ui32 MarkupClusterSize;
    ui32 MarkupImportantSize;

    ui32 CommonSize;
    ui32 CommonImportantSize;

    ui32 SampleClusterMarkedCount;
public:
    TMatching(const TClusterId& sampleClusterId,
              ui32 sampleClusterSize = 0,
              ui32 markupClusterSize = 0,
              ui32 markupImportantSize = 0,
              ui32 commonSize = 0,
              ui32 commonImportantSize = 0,
              ui32 markedCount = 0)
        : SampleClusterId(sampleClusterId)
        , SampleClusterSize(sampleClusterSize)
        , MarkupClusterSize(markupClusterSize)
        , MarkupImportantSize(markupImportantSize)
        , CommonSize(commonSize)
        , CommonImportantSize(commonImportantSize)
        , SampleClusterMarkedCount(markedCount)
    {
        Y_ASSERT(SampleClusterSize >= SampleClusterMarkedCount);
        Y_ASSERT(SampleClusterMarkedCount >= CommonSize);
        Y_ASSERT(MarkupClusterSize >= CommonSize);
        Y_ASSERT(CommonSize >= CommonImportantSize);
        Y_ASSERT(CommonSize >= 1);
        Y_ASSERT(SampleClusterSize >= 1);
    }

    double Recall(bool doRegularize = true) const {
        if (doRegularize && MarkupClusterSize < 2) {
            return 0;
        }

        double recall = (double) (CommonSize - doRegularize) / (MarkupClusterSize - doRegularize);
        Y_ASSERT(recall < 1 + 1e-10 && recall > -1e-10);

        return recall;
    }

    double Precision(double factor = 0, bool doRegularize = true) const {
        if (doRegularize && SampleClusterSize < 2) {
            return 0;
        }

        double expectedCommon = factor * (SampleClusterSize - SampleClusterMarkedCount) + CommonSize;
        double precision = (expectedCommon - doRegularize) / (SampleClusterSize - doRegularize);
        Y_ASSERT(precision < 1 + 1e-10 && precision > -1e-10);

        return precision;
    }

    double Probability(double factor = 0, bool doRegularize = true) const {
        if (doRegularize && SampleClusterSize < 2) {
            return 0;
        }

        double expectedCommon = factor * (SampleClusterSize - SampleClusterMarkedCount) + CommonSize;
        double probability = (expectedCommon - doRegularize) / (SampleClusterSize - doRegularize);
        Y_ASSERT(probability < 1 + 1e-10 && probability > -1e-10);

        return probability;
    }

    double ImportantRecall() const {
        Y_ASSERT(MarkupImportantSize > 0);

        double recall = (double) CommonImportantSize / MarkupImportantSize;
        Y_ASSERT(recall < 1 + 1e-10 && recall > -1e-10);

        return recall;
    }

    ui32 GetCommonSize() const {
        return CommonSize;
    }

    ui32 GetCommonImportantSize() const {
        return CommonImportantSize;
    }

    ui32 GetSampleClusterSize() const {
        return SampleClusterSize;
    }

    ui32 GetMarkupClusterSize() const {
        return MarkupClusterSize;
    }

    ui32 GetMarkupImportantSize() const {
        return MarkupImportantSize;
    }

    const TClusterId& GetSampleClusterId() const {
        return SampleClusterId;
    }

    bool operator > (const TMatching& other) const {
        return CommonSize > other.CommonSize;
    }
};

}
