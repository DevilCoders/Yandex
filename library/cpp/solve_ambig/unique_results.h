#pragma once

#include "order.h"

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>

namespace NSolveAmbig {
    template <class T>
    inline void MakeUniqueResults(TVector<T>& res) {
        if (res.size() > 1) {
            ::StableSort(res.begin(), res.end(), NImpl::TResultWeightedOrder<T>());
            res.erase(::Unique(res.begin(), res.end(), NImpl::TResultEqualIgnoreWeight<T>()), res.end());
        }
    }

}
