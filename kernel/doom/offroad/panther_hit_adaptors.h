#pragma once

#include <kernel/doom/hits/panther_hit.h>
#include <library/cpp/offroad/custom/subtractors.h>

namespace NDoom {


class TPantherHitVectorizer {
public:
    enum {
        TupleSize = 2
    };

    template<class Slice>
    static void Gather(Slice&& slice, TPantherHit* hit) {
        *hit = TPantherHit(slice[0], slice[1]);
    }

    template<class Slice>
    static void Scatter(TPantherHit hit, Slice&& slice) {
        slice[0] = hit.DocId();
        slice[1] = hit.Relevance();
    }
};

using TPantherHitSubtractor = NOffroad::TD1I1Subtractor;


} // namespace NDoom
