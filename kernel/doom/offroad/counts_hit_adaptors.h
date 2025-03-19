#pragma once

#include <kernel/doom/hits/counts_hit.h>
#include <library/cpp/offroad/custom/subtractors.h>

namespace NDoom {


class TCountsHitVectorizer {
public:
    enum {
        TupleSize = 3
    };

    template<class Slice>
    static void Gather(Slice&& slice, TCountsHit* hit) {
        *hit = TCountsHit(
            slice[0],
            static_cast<NDoom::EStreamType>(slice[2]),
            slice[1]
        );
    }

    template<class Slice>
    static void Scatter(const TCountsHit& hit, Slice&& slice) {
        slice[0] = hit.DocId();
        slice[1] = hit.Value();
        slice[2] = static_cast<ui32>(hit.StreamType());
    }
};

using TCountsHitSubtractor = NOffroad::TD2I1Subtractor;


} // namespace NDoom
