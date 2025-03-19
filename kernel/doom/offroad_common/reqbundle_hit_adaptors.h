#pragma once

#include <kernel/doom/hits/reqbundle_hit.h>
#include <library/cpp/offroad/custom/subtractors.h>

namespace NDoom {


class TReqBundleHitVectorizer {
public:
    enum {
        TupleSize = 5
    };

    template<class Slice>
    static void Scatter(const TReqBundleHit& hit, Slice&& slice) {
        slice[0] = hit.DocId();
        slice[1] = hit.Break();
        slice[2] = hit.Word();
        slice[3] = hit.Relevance();
        slice[4] = hit.Form();
        Y_ASSERT(hit.Range() == 0);
    }

    template<class Slice>
    static void Gather(Slice&& slice, TReqBundleHit* hit) {
        *hit = TReqBundleHit(slice[0], slice[1], slice[2], slice[3], slice[4], /*range=*/0);
    }
};

class TReqBundleHitVectorizerV2 {
public:
    enum {
        TupleSize = 6
    };

    template<class Slice>
    static void Scatter(const TReqBundleHit& hit, Slice&& slice) {
        slice[0] = hit.DocId();
        slice[1] = hit.Range();
        slice[2] = hit.Break();
        slice[3] = hit.Word();
        slice[4] = hit.Relevance();
        slice[5] = hit.Form();
    }

    template<class Slice>
    static void Gather(Slice&& slice, TReqBundleHit* hit) {
        *hit = TReqBundleHit(slice[0], slice[2], slice[3], slice[4], slice[5], slice[1]);
    }
};


class TReqBundleHitPrefixVectorizer {
public:
    enum {
        TupleSize = 1
    };

    template<class Slice>
    static void Gather(Slice&& slice, TReqBundleHit* hit) {
        *hit = TReqBundleHit(slice[0], 0, 0, 0, 0, 0);
    }

    template<class Slice>
    static void Scatter(TReqBundleHit hit, Slice&& slice) {
        slice[0] = hit.DocId();
    }
};

class TReqBundleHitPrefixVectorizerV2 {
public:
    enum {
        TupleSize = 2
    };

    template<class Slice>
    static void Gather(Slice&& slice, TReqBundleHit* hit) {
        *hit = TReqBundleHit(slice[0], 0, 0, 0, 0, slice[1]);
    }

    template<class Slice>
    static void Scatter(TReqBundleHit hit, Slice&& slice) {
        slice[0] = hit.DocId();
        slice[1] = hit.Range();
    }
};


using TReqBundleHitSubtractor = NOffroad::TPD1D3I1Subtractor;
using TReqBundleHitSubtractorV2 = NOffroad::TPD2D3I1Subtractor;


} // namespace NDoom
