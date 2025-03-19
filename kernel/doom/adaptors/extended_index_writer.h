#pragma once

#include <util/generic/vector.h>

namespace NDoom {


template<class Base>
class TExtendedIndexWriter: public Base {
public:
    using TKey = typename Base::TKey;
    using TKeyRef = typename Base::TKeyRef;
    using THit = typename Base::THit;

    using Base::Base;

    using Base::WriteHit;
    using Base::WriteKey;

    void WriteHits(const TVector<THit>& hits) {
        for (const THit& hit : hits)
            Base::WriteHit(hit);
    }

    void WriteKey(const TKeyRef& key, const TVector<THit>& hits) {
        Base::WriteKey(key);
        WriteHits(hits);
    }
};


} // namespace NDoom
