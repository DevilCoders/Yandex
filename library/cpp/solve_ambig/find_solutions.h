#pragma once

#include "solution_collect.h"
#include "solution.h"
#include "rank.h"

#include <util/generic/vector.h>

namespace NSolveAmbig {
    // Finds all non-overlapping sequences of occurrences. Returns true if all possible solutions are found and
    // false if the limit is reached
    // Each solution contains sequence of corresponding item positions
    // "items" collection is reordered, but is not changed
    template <class T>
    inline bool FindAllSolutions(TVector<T>& items, TVector<TSolutionPtr>& solutions, const TRankMethod& rankMethod = DefaultRankMethod(), const size_t limit = 50) {
        solutions.clear();
        if (items.size() != 0) {
            return NImpl::TSolutionCollector<T>(items, solutions, rankMethod, limit).Find();
        }
        return true;
    }

}
