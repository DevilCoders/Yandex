#pragma once

#include "matching.h"

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <utility>

#include <util/str_stl.h>

namespace NClusterMetrics {

class TClusterMetricsCalculator {
public:
    template<typename TClusterId>
    static double AlexRecall(const TVector<TMatching<TClusterId> >& sortedMatchings,
                             double factor,
                             bool doRegularization = true)
    {
        double alexRecall = 0;

        double leftProbability = 1;
        size_t position = 0;

        while (position < sortedMatchings.size() && leftProbability > 1e-10) {
            double negativeProbability = 1 - sortedMatchings[position].Probability(factor, doRegularization);

            while (position + 1 < sortedMatchings.size() &&
                    sortedMatchings[position + 1].Recall(doRegularization) == sortedMatchings[position].Recall(doRegularization))
            {
                negativeProbability *= 1 - sortedMatchings[++position].Probability(factor, doRegularization);
            }

            double recallAddition = sortedMatchings[position].Recall(doRegularization) * (1 - negativeProbability) * leftProbability;

            alexRecall += recallAddition;
            leftProbability *= negativeProbability;

            ++position;
        }

        return alexRecall;
    }

    template<typename TClusterId>
    static double SecondaryAlexRecall(const TVector<TMatching<TClusterId> >& sortedMatchings,
                                      double factor,
                                      bool doRegularization = true)
    {
        double sumSingleAlexRecalls = 0;
        double bestSingleAlexRecall = 0;
        for (size_t i = 0; i < sortedMatchings.size(); ++i) {
            const TMatching<TClusterId>& matching = sortedMatchings[i];
            double singleAR = matching.Recall() * matching.Probability(factor, doRegularization);

            sumSingleAlexRecalls += singleAR;
            bestSingleAlexRecall = Max(bestSingleAlexRecall, singleAR);
        }

        return bestSingleAlexRecall ? bestSingleAlexRecall / sumSingleAlexRecalls : 0;
    }

    template<typename TClusterId>
    static double BCubedPrecision(const TVector<TMatching<TClusterId> >& matchings,
                                  double factor,
                                  bool doRegularization = true,
                                  bool useImportant = false)
    {
        ui32 normalizer = 0;
        double precision = 0;
        for (size_t i = 0; i < matchings.size(); ++i) {
            const TMatching<TClusterId>& matching = matchings[i];

            if (doRegularization && matching.GetSampleClusterSize() < 2) {
                continue;
            }

            ui32 weight = useImportant ? matching.GetCommonImportantSize()
                                       : matching.GetCommonSize();

            precision += matching.Precision(factor, doRegularization) * weight;
            normalizer += weight;
        }
        return normalizer ? precision / normalizer : 0;
    }

    template<typename TClusterId>
    static double BCubedRecall(const TVector<TMatching<TClusterId> >& matchings,
                               bool doRegularization = true,
                               bool useImportant = false)
    {
        if (matchings.empty()) {
            return 0.;
        }
        ui32 normalizer = useImportant ? matchings.front().GetMarkupImportantSize()
                                       : matchings.front().GetMarkupClusterSize();

        double recall = 0;
        for (size_t i = 0; i < matchings.size(); ++i) {
            const TMatching<TClusterId>& matching = matchings[i];
            ui32 weight = useImportant ? matching.GetCommonImportantSize()
                                       : matching.GetCommonSize();
            recall += matching.Recall(doRegularization) * weight;

            Y_ASSERT(normalizer == (useImportant ? matching.GetMarkupImportantSize()
                                                : matching.GetMarkupClusterSize()));
        }
        return normalizer ? recall / normalizer : 0;
    }

    static double BCubedF1(double bCubedPrecision, double bCubedRecall) {
        return bCubedPrecision + bCubedRecall
               ? 2 * bCubedPrecision * bCubedRecall / (bCubedPrecision + bCubedRecall)
               : 0;
    }
};

}
