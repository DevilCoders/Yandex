#pragma once

#include "occ_traits.h"

#include <util/generic/strbuf.h>
#include <utility>

namespace NSolveAmbig {
    struct TOccurrence: public std::pair<size_t, size_t> {
        size_t Info;
        double Weight;

        TOccurrence()
            : Info(0)
            , Weight(1.0)
        {
        }

        TOccurrence(const std::pair<size_t, size_t>& p)
            : std::pair<size_t, size_t>(p)
            , Info(0)
            , Weight(1.0)
        {
        }

        size_t Size() const {
            return second - first;
        }

        void Swap(TOccurrence& occ) {
            DoSwap(first, occ.first);
            DoSwap(second, occ.second);
            DoSwap(Info, occ.Info);
            DoSwap(Weight, occ.Weight);
        }
    };

    template <>
    struct TOccurrenceTraits<TOccurrence> {
        inline static TStringBuf GetId(const TOccurrence&) {
            return TStringBuf();
        }
        inline static size_t GetCoverage(const TOccurrence& occ) {
            return occ.Size();
        }
        inline static size_t GetStart(const TOccurrence& occ) {
            return occ.first;
        }
        inline static size_t GetStop(const TOccurrence& occ) {
            return occ.second;
        }
        inline static double GetWeight(const TOccurrence& occ) {
            return occ.Weight;
        }
    };

}
