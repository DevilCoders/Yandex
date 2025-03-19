#pragma once

#include <kernel/doom/hits/superlong_hit.h>
#include <library/cpp/offroad/custom/subtractors.h>

namespace NDoom {


class TSuperlongHitVectorizer {
public:
    enum {
        TupleSize = 5
    };

    template<class Slice>
    static void Scatter(const TSuperlongHit& hit, Slice&& slice) {
        slice[0] = hit.DocId();
        slice[1] = hit.Break();
        slice[2] = hit.Word();
        slice[3] = hit.Relevance();
        slice[4] = hit.Form();
    }

    template<class Slice>
    static void Gather(Slice&& slice, TSuperlongHit* hit) {
        TSuperlongHit result;
        result.SetDocId(slice[0]);
        result.SetBreak(slice[1]);
        result.SetWord(slice[2]);
        result.SetRelevance(slice[3]);
        result.SetForm(slice[4]);
        *hit = result;
    }
};


class TSuperlongHitPrefixVectorizer {
public:
    enum {
        TupleSize = 1
    };

    template<class Slice>
    static void Gather(Slice&& slice, TSuperlongHit* hit) {
        *hit = TSuperlongHit(slice[0], 0, 0, 0, 0);
    }

    template<class Slice>
    static void Scatter(TSuperlongHit hit, Slice&& slice) {
        slice[0] = hit.DocId();
    }
};


using TSuperlongHitSubtractor = NOffroad::TPD1D3I1Subtractor;


} // namespace NDoom
