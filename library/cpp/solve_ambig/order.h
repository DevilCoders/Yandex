#pragma once

#include "occ_traits.h"
#include "occurrence.h"

#include <util/generic/typetraits.h>

namespace NSolveAmbig {
    namespace NImpl {
        template <class T>
        struct TRightBoundaryOrder {
            inline bool operator()(typename TTypeTraits<T>::TFuncParam l, typename TTypeTraits<T>::TFuncParam r) const {
                return TOccurrenceTraits<T>::GetStop(l) == TOccurrenceTraits<T>::GetStop(r)
                           ? TOccurrenceTraits<T>::GetStart(l) < TOccurrenceTraits<T>::GetStart(r)
                           : TOccurrenceTraits<T>::GetStop(l) < TOccurrenceTraits<T>::GetStop(r);
            }
        };

        template <class T>
        struct TResultWeightedOrder {
            inline bool operator()(typename TTypeTraits<T>::TFuncParam l, typename TTypeTraits<T>::TFuncParam r) const {
                if (TOccurrenceTraits<T>::GetStart(l) == TOccurrenceTraits<T>::GetStart(r)) {
                    if (TOccurrenceTraits<T>::GetStop(l) == TOccurrenceTraits<T>::GetStop(r)) {
                        if (TOccurrenceTraits<T>::GetCoverage(l) == TOccurrenceTraits<T>::GetCoverage(r)) {
                            if (TOccurrenceTraits<T>::GetId(l) == TOccurrenceTraits<T>::GetId(r)) {
                                // Result with higher weight should appear first
                                return TOccurrenceTraits<T>::GetWeight(l) > TOccurrenceTraits<T>::GetWeight(r);
                            } else {
                                return TOccurrenceTraits<T>::GetId(l) < TOccurrenceTraits<T>::GetId(r);
                            }
                        } else {
                            return TOccurrenceTraits<T>::GetCoverage(l) < TOccurrenceTraits<T>::GetCoverage(r);
                        }
                    } else {
                        return TOccurrenceTraits<T>::GetStop(l) < TOccurrenceTraits<T>::GetStop(r);
                    }
                } else {
                    return TOccurrenceTraits<T>::GetStart(l) < TOccurrenceTraits<T>::GetStart(r);
                }
            }
        };

        template <class T>
        struct TResultEqualIgnoreWeight {
            inline bool operator()(typename TTypeTraits<T>::TFuncParam l, typename TTypeTraits<T>::TFuncParam r) const {
                return TOccurrenceTraits<T>::GetStart(l) == TOccurrenceTraits<T>::GetStart(r) && TOccurrenceTraits<T>::GetStop(l) == TOccurrenceTraits<T>::GetStop(r) && TOccurrenceTraits<T>::GetCoverage(l) == TOccurrenceTraits<T>::GetCoverage(r) && TOccurrenceTraits<T>::GetId(l) == TOccurrenceTraits<T>::GetId(r);
            }
        };

        struct TOccurrenceInfoOrder {
            inline bool operator()(const TOccurrence& o1, const TOccurrence& o2) const {
                return o1.Info < o2.Info;
            }
        };

    }

}
