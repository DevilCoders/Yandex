#pragma once

#include <kernel/doom/hits/attributes_hit.h>
#include <library/cpp/offroad/custom/subtractors.h>

namespace NDoom {


class TAttributesHitVectorizer {
public:
    enum {
        TupleSize = 1
    };

    template<class Slice>
    static void Gather(Slice&& slice, TAttributesHit* hit) {
        *hit = TAttributesHit(slice[0]);
    }

    template<class Slice>
    static void Scatter(const TAttributesHit& hit, Slice&& slice) {
        slice[0] = hit.DocId();
    }
};

using TAttributesHitSubtractor = NOffroad::TPD1Subtractor;


} // namespace NDoom
