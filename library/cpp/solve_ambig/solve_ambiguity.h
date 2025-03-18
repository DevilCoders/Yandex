#pragma once

#include "solution_order.h"
#include "solution.h"
#include "order.h"
#include "rank.h"
#include "occ_traits.h"
#include "functors.h"

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>

namespace NSolveAmbig {
    template <class T, class TOnDelete>
    inline void SolveAmbiguity(TVector<T>& items, TOnDelete onDelete, const TRankMethod& rankMethod = DefaultRankMethod()) {
        if (items.size() <= 1) {
            return;
        }

        NImpl::TSolutionOrder cmp(rankMethod);
        ::StableSort(items.begin(), items.end(), NImpl::TRightBoundaryOrder<T>());

        TVector<TSolution> solutions(items.size());
        for (size_t i = 0; i < items.size(); ++i) {
            const T& curItem = items[i];

            const TSolution* bestOverlappingSolution = nullptr;
            const TSolution* bestNonOverlappingSolution = nullptr;

            for (int j = i - 1; j >= 0; --j) {
                if (TOccurrenceTraits<T>::GetStop(items[j]) <= TOccurrenceTraits<T>::GetStart(curItem)) { //does not intersect
                    bestNonOverlappingSolution = &(solutions[j]);
                    break;
                } else if (bestOverlappingSolution == nullptr || cmp(solutions[j], *bestOverlappingSolution)) {
                    bestOverlappingSolution = &(solutions[j]);
                }
            }

            TSolution& curSolution = solutions[i];
            if (bestNonOverlappingSolution != nullptr) {
                curSolution = *bestNonOverlappingSolution; // nasty copying!
            }
            curSolution.Coverage += TOccurrenceTraits<T>::GetCoverage(curItem);
            curSolution.Weight *= TOccurrenceTraits<T>::GetWeight(curItem);
            curSolution.Positions.push_back(i);

            // if overlapped solution is still better
            // The construction "!cmp(curSolution, *bestOverlappingSolution)" prioritizes left-hand parses
            if (bestOverlappingSolution != nullptr && !cmp(curSolution, *bestOverlappingSolution)) {
                curSolution = *bestOverlappingSolution; // nasty copying again!
            }
        }
        const TVector<size_t>& bestSolutionIndices = solutions.back().Positions;
        for (size_t i = 0; i < bestSolutionIndices.size(); ++i) {
            if (i != bestSolutionIndices[i]) {
                DoSwap(items[i], items[bestSolutionIndices[i]]);
            }
        }

        for (size_t i = bestSolutionIndices.size(); i < items.size(); ++i) {
            onDelete(items[i]);
        }

        items.resize(bestSolutionIndices.size());
    }

    template <class T>
    inline void SolveAmbiguity(TVector<T>& items, const TRankMethod& rankMethod = DefaultRankMethod()) {
        SolveAmbiguity(items, NImpl::TNop(), rankMethod);
    }

    template <class T>
    inline void SolveAmbiguity(TVector<T>& items, TVector<T>& dropped, const TRankMethod& rankMethod = DefaultRankMethod()) {
        SolveAmbiguity(items, NImpl::TMoveOp<T>(dropped), rankMethod);
    }

    template <class T>
    inline void SolveResultAmbiguity(TVector<T>& results, TVector<TOccurrence>& occurrences, const TRankMethod& rankMethod = DefaultRankMethod()) {
        SolveAmbiguity(occurrences, rankMethod);
        ::StableSort(occurrences.begin(), occurrences.end(), NImpl::TOccurrenceInfoOrder());
        for (size_t i = 0; i < occurrences.size(); ++i) {
            if (i != occurrences[i].Info) {
                DoSwap(results[i], results[occurrences[i].Info]);
            }
        }
        results.resize(occurrences.size());
    }

}
