#pragma once

#include "solution.h"
#include "solution_order.h"
#include "rank.h"
#include "occ_traits.h"

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/utility.h>
#include <util/generic/hash_set.h>
#include <util/digest/numeric.h>
#include <util/digest/sequence.h>

namespace NSolveAmbig {
    namespace NImpl {
        template <class T>
        class TSolutionCollector {
        private:
            TVector<T>& Items;
            TVector<TSolutionPtr>& Solutions;
            size_t Limit;
            bool HasAllResults;
            const TSolutionOrder Cmp;

        private:
            inline void AddSolutionItem(TSolution& solution, size_t itemPos) const {
                solution.Coverage += TOccurrenceTraits<T>::GetCoverage(Items[itemPos]);
                solution.Weight *= TOccurrenceTraits<T>::GetWeight(Items[itemPos]);
                solution.Positions.push_back(itemPos);
            }

            TSolutionPtr TruncateSolutionBeforePos(const TSolution& solution, size_t pos) const {
                TSolutionPtr res;
                // Check that truncated solution will have at least one element
                if (!solution.Positions.empty() && TOccurrenceTraits<T>::GetStop(Items[solution.Positions[0]]) <= pos) {
                    res = new TSolution();
                    AddSolutionItem(*res, solution.Positions[0]);
                    for (size_t i = 1; i < solution.Positions.size() && TOccurrenceTraits<T>::GetStop(Items[solution.Positions[i]]) <= pos; ++i) {
                        AddSolutionItem(*res, solution.Positions[i]);
                    }
                }
                return res;
            }

            // Put a new solution into the list with the limited size. The worst solution is removed if the new size exceeds the limit
            void PutSolution(TVector<TSolutionPtr>& coll, const TSolutionPtr& s) {
                if (coll.size() >= Limit) {
                    HasAllResults = false;
                    if (Cmp(coll.front(), s))
                        return;
                    std::pop_heap(coll.begin(), coll.end(), Cmp);
                    coll.pop_back();
                }
                coll.push_back(s);
                std::push_heap(coll.begin(), coll.end(), Cmp);
            }

            void ExtendSolutions(size_t itemPos) {
                const T& item = Items[itemPos];
                bool createNewSolution = true;                         // Need to create a new solution with the single current item
                THashSet<TVector<size_t>, TSimpleRangeHash> uniqBases; // Unique position sets after truncation
                TVector<TSolutionPtr> newSolutions;
                for (size_t i = 0; i < Solutions.size(); ++i) {
                    if (TOccurrenceTraits<T>::GetStop(Items[Solutions[i]->Positions.back()]) <= TOccurrenceTraits<T>::GetStart(item)) {
                        AddSolutionItem(*Solutions[i], itemPos);
                        PutSolution(newSolutions, Solutions[i]);
                        createNewSolution = false;
                    } else {
                        PutSolution(newSolutions, Solutions[i]);
                        TSolutionPtr newSol = TruncateSolutionBeforePos(*Solutions[i], TOccurrenceTraits<T>::GetStart(item));
                        if (newSol.Get()) {
                            if (uniqBases.insert(newSol->Positions).second) {
                                AddSolutionItem(*newSol, itemPos);
                                PutSolution(newSolutions, newSol);
                            }
                            createNewSolution = false;
                        }
                    }
                }
                if (createNewSolution) {
                    TSolutionPtr newSol(new TSolution());
                    AddSolutionItem(*newSol, itemPos);
                    PutSolution(newSolutions, newSol);
                }
                DoSwap(newSolutions, Solutions);
            }

        public:
            TSolutionCollector(TVector<T>& items, TVector<TSolutionPtr>& solutions, const TRankMethod& rankMethod, size_t limit)
                : Items(items)
                , Solutions(solutions)
                , Limit(limit)
                , HasAllResults(true)
                , Cmp(rankMethod)
            {
            }

            bool Find() {
                ::StableSort(Items.begin(), Items.end(), TRightBoundaryOrder<T>());
                Solutions.push_back(new TSolution());
                AddSolutionItem(*Solutions.back(), 0);

                for (size_t i = 1; i < Items.size(); ++i) {
                    ExtendSolutions(i);
                }
                ::StableSort(Solutions.begin(), Solutions.end(), Cmp);
                return HasAllResults;
            }
        };

    }

}
