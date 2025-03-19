#pragma once

#include <util/system/yassert.h>

#include <library/cpp/offroad/custom/subtractors.h>

#include <kernel/doom/hits/ann_data_hit.h>
#include <kernel/doom/enums/namespace.h>

namespace NDoom {

class TAnnDataHitWadVectorizer {
public:
    enum {
        TupleSize = 4
    };

    template<class Slice>
    static void Scatter(const TAnnDataHit& hit, Slice&& slice) {
        slice[0] = hit.Break();
        slice[1] = hit.Region();
        slice[2] = hit.Stream();
        slice[3] = hit.Value();
    }

    template<class Slice>
    static void Gather(Slice&& slice, TAnnDataHit* hit) {
        hit->SetBreak(slice[0]);
        hit->SetRegion(slice[1]);
        hit->SetStream(slice[2]);
        hit->SetValue(slice[3]);
    }
};


class TAnnDataHitWadPrefixVectorizer {
public:
    enum {
        TupleSize = 1
    };

    template<class Slice>
    static void Scatter(const TAnnDataHit& hit, Slice&& slice) {
        slice[0] = hit.Break();
    }

    template<class Slice>
    static void Gather(Slice&& slice, TAnnDataHit* hit) {
        hit->SetBreak(slice[0]);
    }
};

class TAnnDataHitWithRegionWadPrefixVectorizer {
public:
    enum {
        TupleSize = 2
    };

    template<class Slice>
    static void Scatter(const TAnnDataHit& hit, Slice&& slice) {
        slice[0] = hit.Break();
        slice[1] = hit.Region();
    }

    template<class Slice>
    static void Gather(Slice&& slice, TAnnDataHit* hit) {
        hit->SetBreak(slice[0]);
        hit->SetRegion(slice[1]);
    }
};



using TAnnDataHitWadSubtractor = NOffroad::TPD1D2I1Subtractor;


} // namespace NDoom
