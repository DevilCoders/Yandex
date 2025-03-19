#pragma once

#include <util/system/yassert.h>

#include <library/cpp/offroad/custom/subtractors.h>

#include <kernel/doom/hits/doc_attrs_hit.h>
#include <kernel/doom/enums/namespace.h>

namespace NDoom {

class TDocAttrsHitVectorizer {
public:
    enum {
        TupleSize = 2
    };

    template <class Slice, class CategType>
    static void Scatter(const TGenericDocAttrsHit<CategType>& hit, Slice&& slice) {
        slice[0] = hit.AttrNum();
        slice[1] = hit.Categ();
    }

    template <class Slice, class CategType>
    static void Gather(Slice&& slice, TGenericDocAttrsHit<CategType>* hit) {
        hit->SetAttrNum(slice[0]);
        hit->SetCateg(slice[1]);
    }
};

class TDocAttrs64HitVectorizer {
public:
    enum {
        TupleSize = 3
    };

    template <class Slice>
    static void Scatter(const TDocAttrs64Hit& hit, Slice&& slice) {
        slice[0] = hit.AttrNum();
        slice[1] = hit.Categ();
        slice[2] = hit.Categ() >> 32;
    }

    template <class Slice>
    static void Gather(Slice&& slice, TDocAttrs64Hit* hit) {
        hit->SetAttrNum(slice[0]);
        hit->SetCateg(slice[1] | (static_cast<ui64>(slice[2]) << 32));
    }
};

class TDocAttrsHitPrefixVectorizer {
public:
    enum {
        TupleSize = 1
    };

    template <class Slice, class CategType>
    static void Scatter(const TGenericDocAttrsHit<CategType>& hit, Slice&& slice) {
        slice[0] = hit.AttrNum();
    }

    template <class Slice, class CategType>
    static void Gather(Slice&& slice, TGenericDocAttrsHit<CategType>* hit) {
        hit->SetAttrNum(slice[0]);
    }
};

using TDocAttrsHitSubtractor = NOffroad::TPD1D1Subtractor;
using TDocAttrs64HitSubtractor = NOffroad::TPD1D2Subtractor;

} // namespace NDoom
